cmake_minimum_required(VERSION 3.17)
project(Server C)

set(CMAKE_C_STANDARD 11)

find_package(Threads REQUIRED)
find_library(GMP_LIBRARIES gmp PATHS $ENV{GMPDIR} ${LIB_INSTALL_DIR})

add_executable(Server start_routine.c
        ServerActions/server_actions.c ServerActions/server_actions.h
        ClientLists/client_lists.c ClientLists/client_lists.h
        ../Utilities/common_func.h ../Utilities/common_func.c
        ../Crypto/rsa.h ../Crypto/rsa.c)

target_link_libraries(Server ${CMAKE_THREAD_LIBS_INIT} ${GMP_LIBRARIES})
#set (CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -g -fno-omit-frame-pointer -fsanitize=address")

