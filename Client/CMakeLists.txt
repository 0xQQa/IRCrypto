cmake_minimum_required(VERSION 3.17)
project(Client C)

set(CMAKE_C_STANDARD 11)

find_library(GMP_LIBRARIES gmp PATHS $ENV{GMPDIR} ${LIB_INSTALL_DIR})
find_package(Threads REQUIRED)
find_package(Curses REQUIRED)

add_executable(Client start_routine.c
        ClientActions/client_actions.c ClientActions/client_actions.h
        ../Crypto/rsa.c ../Crypto/rsa.h
        ../Utilities/common_func.c ../Utilities/common_func.h)

target_link_libraries(Client ${CMAKE_THREAD_LIBS_INIT} ${GMP_LIBRARIES} ${CURSES_LIBRARIES})
#set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fno-omit-frame-pointer -fsanitize=address")