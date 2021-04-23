cmake_minimum_required(VERSION 3.13.1)

set(BOARD mimxrt1020_evk)

find_package(Zephyr)

project(dutchess)

target_sources(app PRIVATE src/main.c)
