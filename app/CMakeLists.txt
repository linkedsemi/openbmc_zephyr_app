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

# --- Self-test: fake PECI CPU temperature/power sensor (hwmon_i2c -> IntelCPUSensor) ---
#   west build -b <board> app -DFAKE_PECI_CPUTEMP_TEST=y
# (Can be combined with -DFAKE_TEMP_TEST=y / -DFAKE_PSU_TEST=y.)
if(FAKE_PECI_CPUTEMP_TEST)
  set(CONFIG_FAKE_PECI_CPUTEMP y CACHE BOOL "fake peci cpu sensor" FORCE)
  list(APPEND EXTRA_DTC_OVERLAY_FILE
       "${CMAKE_CURRENT_LIST_DIR}/boards/lsqsh_evb_cpu1_fake_peci_cputemp.overlay")
endif()

# --- Real Intel PECI CPU sensor (hwmon_i2c -> IntelCPUSensor) ---
# Build-switch (the -D...=y kind) is reserved for fake self-test sensors
# only. The real driver is enabled exclusively from prj.conf so the build
# command stays the same once the hardware is wired up -- no need to
# remember to pass a flag. To turn it on, add to prj.conf:
#
#   CONFIG_SENSOR_INTEL_PECI=y
#   CONFIG_PECI=y
#   CONFIG_PECI_LS=y         # linkedsemi PECI controller driver
#
# The matching DT overlay is applied automatically below (post
# find_package(Zephyr), where the Kconfig cache var is visible to CMake).
# Mutually exclusive with -DFAKE_PECI_CPUTEMP_TEST=y because both define
# a hwmon node with the same "peci_cputemp" label.

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# Real Intel PECI: apply the matching DT overlay whenever the user has
# enabled CONFIG_SENSOR_INTEL_PECI in prj.conf. Doing it here (after
# find_package) means the Kconfig cache var is resolved and visible.
if(CONFIG_SENSOR_INTEL_PECI)
  if(FAKE_PECI_CPUTEMP_TEST)
    message(FATAL_ERROR
      "CONFIG_SENSOR_INTEL_PECI (prj.conf) and FAKE_PECI_CPUTEMP_TEST "
      "(-D...=y) are mutually exclusive: both define a hwmon node with "
      "the same \"peci_cputemp\" label. Pick one.")
  endif()
  list(APPEND EXTRA_DTC_OVERLAY_FILE
       "${CMAKE_CURRENT_LIST_DIR}/boards/lsqsh_evb_cpu1_intel_peci.overlay")
endif()

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

# Self-test fake PECI CPU temperature sensor (hwmon_i2c -> IntelCPUSensor path).
if (CONFIG_FAKE_PECI_CPUTEMP)
  target_sources(app PRIVATE src/fake_peci_cputemp.c)
endif()

# Real Intel PECI CPU sensor (hwmon_i2c -> IntelCPUSensor path).
# Requires the PECI controller driver to be enabled (CONFIG_PECI=y +
# CONFIG_PECI_LS=y on linkedsemi platforms) and the board's &peci1/&peci2
# nodes status="okay" -- the bundled overlay does that automatically.
if (CONFIG_SENSOR_INTEL_PECI)
  target_sources(app PRIVATE src/intel_peci.c)
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
