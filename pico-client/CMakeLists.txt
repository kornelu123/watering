cmake_minimum_required(VERSION 3.13)

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include(${PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)
include(${CMAKE_SOURCE_DIR}/../common_data_import.cmake)

project(watering C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

include_directories(common/)
include_directories(../common/)

add_subdirectory(bootloader/)
add_subdirectory(waterer/)

set(COMBINED_UF2 ${CMAKE_BINARY_DIR}/combined.uf2)

set(PICO_PRINTF_PICO 1)  # Use smaller printf
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Os")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Os")

add_compile_options(
    -ffunction-sections
    -fdata-sections
    -fno-common
    -fstack-reuse=all
    -fno-exceptions  # If not using C++ exceptions
)

add_custom_command(
    OUTPUT ${COMBINED_UF2}
    COMMAND ${CMAKE_SOURCE_DIR}/tools/combine.sh ${BOOTLOADER_ORIGIN} ${SLOT0_ORIGIN}
                                                 ${SLOT1_ORIGIN} ${SLOT2_ORIGIN}
                                                 ${CRC_SEED}
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
