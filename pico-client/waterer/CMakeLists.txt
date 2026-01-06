function(create_slot_target slot_name flash_offset)
  set(
    FLASH_ORIGIN ${flash_offset}
  )
  message(STATUS "Creating slot: ${slot_name} at offset ${flash_offset}, FLASH_OFFSET: ${FLASH_ORIGIN}")

  set(target_name "watering_${slot_name}")

  add_executable(${target_name}
    main.c
    sched.c
  )

  pico_enable_stdio_usb(${target_name} 1)
  pico_enable_stdio_uart(${target_name} 0)

  target_include_directories(${target_name} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
  )

  target_link_libraries(${target_name}
    pico_cyw43_arch_lwip_threadsafe_background
    pico_stdlib
  )

  # Create linker script with correct origin
  set(linker_script "${CMAKE_CURRENT_BINARY_DIR}/watering_${slot_name}.ld")
  set(ORIGIN ${flash_offset})
  configure_file(
    "${CMAKE_SOURCE_DIR}/waterer/waterer.ld.in"
    "${linker_script}"
    @ONLY
  )

  # Use the custom linker script
  pico_set_linker_script(${target_name} "${linker_script}")

  # Add extra outputs
  pico_add_extra_outputs(${target_name})

  # Rename output files
  set_target_properties(${target_name} PROPERTIES
    OUTPUT_NAME "watering_${slot_name}"
  )

  target_compile_definitions(${target_name} PUBLIC
    CUR_SLOT_ORIGIN=${flash_offset}
    WIFI_SSID="${WIFI_SSID}"
    WIFI_PASS="${WIFI_PASS}"
    TEST_TCP_SERVER_IP="${TEST_TCP_SERVER_IP}"
  )
  unset(
      FLASH_ORIGIN
  )
endfunction()

pico_sdk_init()

set(SLOT0_ORIGIN 0x10020000)
set(SLOT1_ORIGIN 0x100A0000)
set(SLOT2_ORIGIN 0x10120000)

set(
    PICO_NO_BINARY_INFO 1
    PICO_NO_FLASH 1
)

foreach(slot_name slot0 slot1 slot2)
  if(${slot_name} STREQUAL "slot0")
    set(slot_offset ${SLOT0_ORIGIN})
  elseif(${slot_name} STREQUAL "slot1")
    set(slot_offset ${SLOT1_ORIGIN})
  else()
    set(slot_offset ${SLOT2_ORIGIN})
  endif()

  create_slot_target(${slot_name} ${slot_offset})
endforeach()
