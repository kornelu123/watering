cmake_minimum_required(VERSION 3.13)

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# Check if working directory is dirty
execute_process(
    COMMAND git diff --quiet --exit-code
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE GIT_DIRTY_STATUS
)

if(GIT_DIRTY_STATUS EQUAL 0)
    set(GIT_DIRTY "clean")
else()
    set(GIT_DIRTY "dirty")
endif()

message(STATUS "Git Commit: ${GIT_COMMIT_HASH} (${GIT_DIRTY})")

project(watering_server LANGUAGES C CXX ASM VERSION 0.0.1)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_SOURCE_DIR}/../common_data_import.cmake)

add_executable(${PROJECT_NAME}
  main.c
  logger.c
  serv.c
  recver.c
  controller.c
  creators.c
  cloud.c
  ${CMAKE_SOURCE_DIR}/../common/helpers.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_SOURCE_DIR}/../common
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  BOOTLOADER_ORIGIN=${BOOTLOADER_ORIGIN}
  SLOT0_ORIGIN=${SLOT0_ORIGIN}
  SLOT1_ORIGIN=${SLOT1_ORIGIN}
  SLOT2_ORIGIN=${SLOT2_ORIGIN}
  GIT_DIRTY="${GIT_DIRTY}"
  GIT_COMMIT_HASH="${GIT_COMMIT_HASH}"
  PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
  PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

find_package(CURL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl)
