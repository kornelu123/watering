cmake_minimum_required(VERSION 3.13)

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include(${PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

project(watering C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

include_directories(common/)

set(BOOTLOADER_ORIGIN   0x10000000)
set(SLOT0_ORIGIN        0x10020000)
set(SLOT1_ORIGIN        0x100A0000)
set(SLOT2_ORIGIN        0x10120000)

add_subdirectory(bootloader/)
add_subdirectory(waterer/)

set(COMBINED_UF2 ${CMAKE_BINARY_DIR}/combined.uf2)

add_custom_command(
    OUTPUT ${COMBINED_UF2}
    COMMAND ${CMAKE_SOURCE_DIR}/tools/combine.sh ${BOOTLOADER_ORIGIN} ${SLOT0_ORIGIN}
                                                 ${SLOT1_ORIGIN} ${SLOT2_ORIGIN}
    DEPENDS
        ${CMAKE_BINARY_DIR}/bootloader/bootloader.elf
        ${CMAKE_BINARY_DIR}/waterer/watering_slot0.elf
        ${CMAKE_BINARY_DIR}/waterer/watering_slot1.elf
        ${CMAKE_BINARY_DIR}/waterer/watering_slot2.elf
    COMMENT "Creating combined UF2 file"
    VERBATIM
)

add_custom_target(combined ALL DEPENDS ${COMBINED_UF2})

file(COPY ${CMAKE_SOURCE_DIR}/tools/combine.sh
     DESTINATION ${CMAKE_BINARY_DIR}/tools/
     FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
