#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "sched.h"
#include "tcp_server.h"


#define WIFI_SSID "PicoNet"
#define WIFI_PASSWORD "MosKonewka"

int main() {
    stdio_init_all();

    if (cyw43_arch_init_with_country(CYW43_COUNTRY_POLAND)) {
        return 1;
    }
    
    cyw43_arch_enable_ap_mode(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);
    
    // IP config
    struct netif *n = &cyw43_state.netif[CYW43_ITF_AP];
    ip4_addr_t ip, netmask, gw;
    IP4_ADDR(&ip, 192, 168, 4, 1);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 192, 168, 4, 1);
    netif_set_addr(n, &ip, &netmask, &gw);
    netif_set_up(n);
    
    tcp_server_init();

    __run_sched();
    return 0;
}