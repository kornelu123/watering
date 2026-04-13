/** * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "pico/cyw43_arch.h"
#include "pico/stdio.h"

#include "dispatcher.h"
#include "sched.h"
#include "proto.h"
#include "shared_mem.h"
#include "gpio_irq.c"

#include "tcp_client.h"

#define CLOUD_IP "192.168.1.1"
#define DISPLAYER_IP "192.168.4.1" 

#if !defined(PROJECT_VERSION_MAJOR) || !defined(PROJECT_VERSION_MINOR)|| !defined(PROJECT_VERSION_PATCH)
  #error "PROJECT_VERSION cannot be read"
#endif

void
__main(void) {
  if (!tcp_client_init_and_connect(CLOUD_IP, &cloud_tcp)) {
      printf("Warning: No connection established, continuing offline.\n");
  }

  if (!tcp_client_init_and_connect(DISPLAYER_IP, &display_tcp)) {
      printf("Warning: Displayer connection failed, continuing offline.\n");
  }
  
  __run_sched();
}

void __attribute__((__noreturn__)) 
main()
{
  stdio_init_all();

  if (cyw43_arch_init()) {
    exit(-1);
  }

  cyw43_arch_enable_sta_mode();

  while (1) {
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Wi-Fi connection failed, retrying.\n");
    }
    else {
      printf("Connected.\n");
      cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
      break;
    }
  }

  printf("Welcome in waterer software!\n");
  printf("Running at %08X\n", (uint32_t)CUR_SLOT_ORIGIN);
  printf("Running slot  in flash: %02X\n", get_running_slot_id());
  printf("Running firmware ver: %02X.%02X.%02X\n",
         PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH);
  printf("Connecting to %s Wi-Fi...\n", WIFI_SSID);

  // Initializes GPIO interrupts required for system wake-up.
  init_gpio();

  __main();

  cyw43_arch_deinit();
  exit(0);
}