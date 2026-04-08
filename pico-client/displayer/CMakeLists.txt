cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)

# Initializes the Pico SDK if this is built as a standalone project.
if (NOT TARGET pico_stdlib)
    pico_sdk_init()
endif()

add_executable(displayer
    main.c
    sched.c
    ssd1680.c
    tcp_server.c
    text_renderer.c
)

# Routes stdout to USB for debugging without requiring a UART adapter.
pico_enable_stdio_usb(displayer 1)
pico_enable_stdio_uart(displayer 0)

target_include_directories(displayer PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
    ${CMAKE_CURRENT_LIST_DIR}/../common/
    ${CMAKE_CURRENT_LIST_DIR}/../../common/
)

# Links standard SDK libraries alongside hardware-specific modules used by the E-ink and Watchdog.
target_link_libraries(displayer
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip
    hardware_spi
    hardware_watchdog
    hardware_timer
    hardware_irq
)

# For the .uf2 file
pico_add_extra_outputs(displayer)