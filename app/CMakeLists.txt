# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20.0)

# --- Self-test: fake temperature sensor (hwmon_i2c -> HwmonTempSensor) ---
# Enable both the fake sensor driver (CONFIG_FAKE_TEMP_SENSOR) and its DT
# overlay with a single flag:
#   west build -b <board> app -DFAKE_TEMP_TEST=y
# Leave off for production builds. MUST be set before find_package(Zephyr)
# because the overlay is consumed during devicetree generation (Kconfig runs
# first inside find_package, so the CONFIG cache var is picked up too).
if(FAKE_TEMP_TEST)
  set(CONFIG_FAKE_TEMP_SENSOR y CACHE BOOL "fake temp sensor" FORCE)
  set(EXTRA_DTC_OVERLAY_FILE
      "${CMAKE_CURRENT_LIST_DIR}/boards/lsqsh_evb_cpu1_fake_temp.overlay"
      CACHE FILEPATH "fake temp test overlay" FORCE)
endif()

# --- Self-test: fake PSU sensor (hwmon_i2c -> PSUSensor) ---
#   west build -b <board> app -DFAKE_PSU_TEST=y
# (Can be combined with -DFAKE_TEMP_TEST=y: the overlay is appended to the
# list so both apply. Set before find_package(Zephyr), same reason as above.)
if(FAKE_PSU_TEST)
  set(CONFIG_FAKE_PSU_SENSOR y CACHE BOOL "fake psu sensor" FORCE)
  list(APPEND EXTRA_DTC_OVERLAY_FILE
       "${CMAKE_CURRENT_LIST_DIR}/boards/lsqsh_evb_cpu1_fake_psu.overlay")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(openbmc LANGUAGES C CXX)

target_include_directories(app PRIVATE include)

target_sources(app PRIVATE
  src/main.cpp
  src/thread_dependency_mgr.cpp
  src/task_def.cpp
)

target_sources(app PRIVATE
  src/sys_mmap_impl.c
#   src/test_deployment.c
  src/test_sd_event.c
  # ${CMAKE_CURRENT_LIST_DIR}/../../samples/test_event/sd_event_test.c
  src/misc.c
  src/net_stack_cfg.c
  src/dbroker.c
  src/socketpool.c
  src/config_fs.c
  src/busctl_shell.c
)

# Self-test fake temperature sensor (hwmon_i2c -> HwmonTempSensor path).
if (CONFIG_FAKE_TEMP_SENSOR)
  target_sources(app PRIVATE src/fake_temp.c)
endif()

# Self-test fake PSU sensor (hwmon_i2c -> PSUSensor path).
if (CONFIG_FAKE_PSU_SENSOR)
  target_sources(app PRIVATE src/fake_psu.c)
endif()

# Handle Unix socket compatibility
target_compile_definitions(app PRIVATE
-D__ZEPHYR__
-D_POSIX_SOCKETS
-D_AF_UNIX_EXISTS_=1
)

zephyr_compile_options(-msmall-data-limit=0 -Wno-strict-prototypes -Wno-macro-redefinition)
target_link_options(app PRIVATE -Wl,--no-relax)
# Route every abort() call through our __wrap_abort() so a silent
# c_assert/abort() inside dbus-broker (which would otherwise kill the
# broker thread with no log) is captured via printk.
target_link_options(app PRIVATE -Wl,--wrap=abort)

if (CONFIG_ZEPHYRBMC_FILESYSTEM)
zephyr_library_link_libraries(filesystem)
endif()

zephyr_library_link_libraries(
	dbus-broker
	basu
  phosphor-objmgr
)

# zephyr_library_link_libraries(sd-bus)
zephyr_library_link_libraries(zephyr-osal)
zephyr_library_link_libraries(boost)
# zephyr_library_link_libraries(zephyr-test)
# zephyr_library_link_libraries(phosphor-user-manager)

zephyr_compile_definitions(
    _GNU_SOURCE
    # SDBUSPLUS_DISABLE_BOOST_COROUTINES
    _GLIBCXX_HAVE_ALIGNED_ALLOC
    _GLIBCXX_HAVE_STRTOLD
    _GLIBCXX_HAVE_DIRENT_H
    _GTHREAD_USE_MUTEX_INIT_FUNC
    _GTHREAD_USE_RECURSIVE_MUTEX_INIT_FUNC
    _GTHREAD_USE_COND_INIT_FUNC
    _GLIBCXX_HAVE_TIMESPEC_GET
)
