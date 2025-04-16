cmake_minimum_required(VERSION 3.30.3)

project(durak_server)

set(SOURCES
    cards.c
    game.c
    session.c
    server.c
    main.c
)

add_executable(server ${SOURCES})
