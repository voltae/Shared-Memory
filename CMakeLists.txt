cmake_minimum_required(VERSION 3.9)
project(Shared-Memory C)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wextra -Wstrict-prototypes -Wformat=2 -pedantic -fno-common -ftrapv -O3 -g -std=gnu11")

set(CMAKE_C_STANDARD 11)

add_executable(sender sender.c)
target_link_libraries(sender pthread rt)
add_executable(receivertest receiver_test.c)
target_link_libraries(receivertest pthread rt)