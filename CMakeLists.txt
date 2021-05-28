cmake_minimum_required(VERSION 3.13.1)

set(BOARD mimxrt1020_evk)

include_directories(
    lib/civetweb_libc_extensions/include
)

find_package(Zephyr)

project(DUTCHess)

target_sources(app PRIVATE src/main.c
                           src/led.c
                           src/relay.c
                           src/gpio.c
                           src/web.c
                           src/shell.c
                           src/fs.c
                           src/net.c
                           src/settings.c
                           src/terminal_server.c
                           lib/civetweb_libc_extensions/src/libc_extensions.c)
