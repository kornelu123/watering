cmake_minimum_required(VERSION 3.13)

project(watering_server C CXX ASM)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_SOURCE_DIR}/../common_data_import.cmake)

add_executable(${PROJECT_NAME}
  main.c
  dispatcher.c
  creators.c
  controller.c
  ${CMAKE_SOURCE_DIR}/../common/helpers.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_SOURCE_DIR}/../common
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  CRC_SEED=${CRC_SEED}
  BOOTLOADER_ORIGIN=${BOOTLOADER_ORIGIN}
  SLOT0_ORIGIN=${SLOT0_ORIGIN}
  SLOT1_ORIGIN=${SLOT1_ORIGIN}
  SLOT2_ORIGIN=${SLOT2_ORIGIN}
)
