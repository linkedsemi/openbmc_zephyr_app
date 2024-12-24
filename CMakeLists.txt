
cmake_minimum_required(VERSION 3.20.0)

set(TOOLCHAIN_HAS_NEWLIB y)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(bmc_app LANGUAGES C CXX)

target_sources(app PRIVATE main.c
    dbus/sd-bus.c
    dbus/sd-event.c
    dbus/sd-journal.c
    misc.c)
zephyr_include_directories(inc)
zephyr_linker_sources(SECTIONS ipmid_section.ld)
