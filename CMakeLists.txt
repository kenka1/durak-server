cmake_minimum_required(VERSION 3.30.3)

project(durak_server)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -g -fsanitize=address -fsanitize=undefined")

set(SOURCES
    cards.c
    game.c
    session.c
    server_init.c
    server.c
    main.c
)

add_executable(server ${SOURCES})
