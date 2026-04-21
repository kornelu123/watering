#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <pthread.h>

#include "controller.h"
#include "ring_buf.h"
#include "logger.h"
#include "serv.h"

DEFINE_RINGBUF(packet, intern_packet_t);

pico_ctx_t *pico_ctxs = NULL;
volatile uint32_t pico_count = 0;

pthread_cond_t  packet_buf_cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t packet_buf_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t  control_buf_cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t control_buf_mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast_packet(void)
{
    for (int i=0; i<pico_count; i++) {
        send_packet(&(pico_ctxs[i]));
    }
}

void send_packet(pico_ctx_t *pico_ctx)
{
    packet_t *out_packet = &(pico_ctx->out_buf);
    pico_ctx->last_msg_id = out_packet->header.msg_id;

    send(pico_ctx->fd, (const void *)out_packet,
         out_packet->header.length + sizeof(header_t),
         MSG_CONFIRM);
}

void add_to_ctxs(const int fd)
{
    pico_ctxs = (pico_ctx_t *)realloc(pico_ctxs, sizeof(pico_ctx_t)*(pico_count + 1));

    pico_ctxs[pico_count].fd = fd;
    pico_ctxs[pico_count].last_msg_id = 0;
    pico_ctxs[pico_count].cur_slot_id = 0xFF;
    pico_ctxs[pico_count].intern_state = INIT;
    pico_ctxs[pico_count].recv_len = 0;
    memset(pico_ctxs[pico_count].recv_buf, 0, MAX_DATA_LEN);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    pico_ctxs[pico_count].status_deadline = ts.tv_sec;

    pthread_mutex_init(&pico_ctxs[pico_count].pico_mut, NULL);

    wakeup_controller();
    pico_count++;
}

int del_from_ctxs(const int fd)
{
    int i;
    for (i=0; i<pico_count; i++) {
        if (pico_ctxs[i].fd == fd) break;
    }
    if (i == pico_count) return -1;

    pthread_mutex_destroy(&pico_ctxs[i].pico_mut);

    if (i != pico_count -1) {
        memcpy((void *)&pico_ctxs[i], (void *)&pico_ctxs[pico_count-1], sizeof(pico_ctx_t));
    }

    pico_count--;
    pico_ctxs = realloc(pico_ctxs, sizeof(pico_ctx_t)*pico_count);
    return 0;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int get_listener_socket(void)
{
    int listener;
    int yes=1;
    int rv;
    struct addrinfo hints, *ai, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        log_err("selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) continue;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }
        break;
    }
    freeaddrinfo(ai);
    if (p == NULL) return -1;
    if (listen(listener, 10) == -1) return -1;
    return listener;
}

void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size)
{
    if (*fd_count == *fd_size) {
        *fd_size *= 2;
        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }
    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN;
    add_to_ctxs(newfd);
    (*fd_count)++;
    wakeup_controller();
}

void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    del_from_ctxs(pfds[i].fd);
    pfds[i] = pfds[*fd_count-1];
    (*fd_count)--;
    wakeup_controller();
}

static int try_extract_packet(pico_ctx_t *ctx)
{
    size_t offset = 0;
    int packets_parsed = 0;

    while (offset + sizeof(header_t) <= ctx->recv_len) {
        header_t *hdr = (header_t *)(ctx->recv_buf + offset);
        size_t total_needed = sizeof(header_t) + hdr->length;

        if (offset + total_needed > ctx->recv_len) {
            break;
        }

        intern_packet_t ipack;
        ipack.fd = ctx->fd;
        ipack.size = total_needed;
        memcpy(&ipack.pack, ctx->recv_buf + offset, total_needed);

        pthread_mutex_lock(&packet_buf_mutex);
        if (packet_ringbuf_push(&ipack) != 0) {
            log_err("No more space in buffer, ignoring packet\n");
        } else {
            pthread_cond_broadcast(&packet_buf_cond);
        }
        pthread_mutex_unlock(&packet_buf_mutex);

        offset += total_needed;
        packets_parsed++;
    }

    if (offset > 0) {
        if (offset < ctx->recv_len) {
            memmove(ctx->recv_buf, ctx->recv_buf + offset, ctx->recv_len - offset);
        }
        ctx->recv_len -= offset;
    }

    return packets_parsed;
}

void *serv_main(void *ptr)
{
    int listener;
    int newfd;
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen;
    char remoteIP[INET6_ADDRSTRLEN];

    int fd_count = 0;
    int fd_size = 5;
    struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

    listener = get_listener_socket();
    if (listener == -1) {
        log_err("error getting listening socket\n");
        exit(1);
    }

    pfds[0].fd = listener;
    pfds[0].events = POLLIN;
    fd_count = 1;

    for(;;) {
        int poll_count = poll(pfds, fd_count, -1);
        if (poll_count == -1) {
            perror("poll");
            exit(1);
        }

        for(int i = 0; i < fd_count; i++) {
            if (pfds[i].revents & POLLIN) {
                if (pfds[i].fd == listener) {
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        add_to_pfds(&pfds, newfd, &fd_count, &fd_size);
                        log_info("New connection from %s on socket %d\n",
                                 inet_ntop(remoteaddr.ss_family,
                                           get_in_addr((struct sockaddr*)&remoteaddr),
                                           remoteIP, INET6_ADDRSTRLEN),
                                 newfd);
                    }
                } else {
                    int sender_fd = pfds[i].fd;

                    pico_ctx_t *ctx = NULL;
                    for (int j = 0; j < pico_count; j++) {
                        if (pico_ctxs[j].fd == sender_fd) {
                            ctx = &pico_ctxs[j];
                            break;
                        }
                    }
                    if (!ctx) {
                        log_err("No context for fd %d\n", sender_fd);
                        continue;
                    }

                    uint8_t tmp[4096];
                    int nbytes = recv(sender_fd, tmp, sizeof(tmp), 0);

                    if (nbytes <= 0) {
                        if (nbytes == 0) {
                            log_info("Socket %d hung up\n", sender_fd);
                        } else {
                            perror("recv");
                        }
                        close(sender_fd);
                        del_from_pfds(pfds, i, &fd_count);
                    } else {
                        if (ctx->recv_len + nbytes > MAX_DATA_LEN) {
                            log_err("Receive buffer overflow for fd %d\n", sender_fd);
                            close(sender_fd);
                            del_from_pfds(pfds, i, &fd_count);
                            continue;
                        }
                        memcpy(ctx->recv_buf + ctx->recv_len, tmp, nbytes);
                        ctx->recv_len += nbytes;

                        int extracted = try_extract_packet(ctx);
                        if (extracted > 0) {
                            log_info("Extracted %d complete packet(s)\n", extracted);
                        }
                    }
                }
            }
        }
    }
    return NULL;
}
