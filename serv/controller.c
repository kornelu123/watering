#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <termios.h>

#include "dispatcher.h"
#include "controller.h"
#include "creators.h"

pthread_t controller_thread;

  void
init_controller(void)
{
  pthread_create(&controller_thread, NULL, &controller_work, NULL);
}

  void *
controller_work(void *data)
{
  struct termios old = {0};

  if (tcgetattr(0, &old) < 0)
    perror("tcsetattr()");

  old.c_lflag &= ~ICANON;  // Disable canonical mode
  old.c_lflag &= ~ECHO;     // Disable echo (optional)
  old.c_cc[VMIN] = 1;       // Read 1 character at a time
  old.c_cc[VTIME] = 0;      // No timeout

  if (tcsetattr(0, TCSANOW, &old) < 0)
  {
    perror("tcsetattr ICANON");
  }

  read_binary_from_file("test_file.bin", 1);

  while (true)
  {
    char inp = fgetc(stdin);

    scanf("%c", &inp);

    switch (inp)
    {
      case 'r': 
        {
          for (int i=0; i<pico_count; i++) {
            create_get_running_slot_packet(3, &(pico_ctxs[i]));
          }

          printf("Broadcasting packet get running slot\n");
          fflush(stdout);
          broadcast_packet();
          break;
        }
      case 'u':
        {
          for (int i=0; i<pico_count; i++) {
            create_update_procedure(pico_ctxs->slot_to_update, &(pico_ctxs[i]));
          }

          broadcast_packet();
          break;
        }
      case 's': 
        {
          for (int i=0; i<pico_count; i++) {
            create_set_active_slot_packet(1, &(pico_ctxs[i]));
          }

          printf("Broadcasting packet set slot\n");
          fflush(stdout);
          broadcast_packet();
          break;
        }
      case 'R': 
        {
          for (int i=0; i<pico_count; i++) {
            create_reset_packet(0xDEAD, &(pico_ctxs[i]));
          }

          printf("Broadcasting packet set slot\n");
          fflush(stdout);
          broadcast_packet();
          break;
        }
      default:
        {
          printf("Unsupported value\n");
          break;
        }
    }
  }
}
