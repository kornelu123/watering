cmake_minimum_required(VERSION 3.13)
project(watering_server_tests LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

enable_testing()

function(add_server_test name)
  add_executable(${name} ${ARGN})
  target_include_directories(${name} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common
  )
  target_compile_options(${name} PRIVATE -Wall -Wextra -Werror)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

add_server_test(test_ring_buf test_ring_buf.c)

add_server_test(test_helpers
  test_helpers.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../../common/helpers.c
)

add_server_test(test_creators
  test_creators.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../creators.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../../common/helpers.c
)
target_compile_definitions(test_creators PRIVATE
  SLOT0_ORIGIN=0x10000000
  SLOT1_ORIGIN=0x10080000
  SLOT2_ORIGIN=0x10100000
)
