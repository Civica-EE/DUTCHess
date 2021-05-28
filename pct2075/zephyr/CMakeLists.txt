# SPDX-License-Identifier: Apache-2.0

if(CONFIG_PCT2075_DRIVER)
  zephyr_include_directories(.)
  zephyr_library()
  zephyr_library_sources(pct2075.c)
endif()
