project(bootloader C CXX ASM)

add_executable(${PROJECT_NAME}
  main.c
  syscalls.c
)

pico_set_linker_script(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/bootloader/bootloader.ld)

target_link_libraries(${PROJECT_NAME}
  hardware_clocks
  hardware_watchdog
  hardware_resets
  hardware_dma
  pico_platform
  pico_crt0
  pico_standard_link
  pico_runtime_headers
  cmsis_core
)

pico_minimize_runtime(${PROJECT_NAME} EXCLUDE ALL)

pico_add_extra_outputs(${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-Wall")

target_compile_definitions(${PROJECT_NAME} PRIVATE
  CUR_SLOT_ORIGIN=${BOOTLOADER_ORIGIN}
  BOOTLOADER_ORIGIN=${BOOTLOADER_ORIGIN}
  CRC_SEED=${CRC_SEED}
  SLOT0_ORIGIN=${SLOT0_ORIGIN}
  SLOT1_ORIGIN=${SLOT1_ORIGIN}
  SLOT2_ORIGIN=${SLOT2_ORIGIN}
)
