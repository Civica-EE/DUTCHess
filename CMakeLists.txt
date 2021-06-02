list(APPEND ZEPHYR_EXTRA_MODULES
  ${CMAKE_CURRENT_SOURCE_DIR}/pct2075_module
)

cmake_minimum_required(VERSION 3.13.1)

set(BOARD mimxrt1020_evk)

include_directories(
    lib/civetweb_libc_extensions/include
)

find_package(Zephyr)

project(DUTCHess)

target_sources(app PRIVATE
    src/main.c
    src/web.c
    src/dut_serial.c
    lib/civetweb_libc_extensions/src/libc_extensions.c
)
