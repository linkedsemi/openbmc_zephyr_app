# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(openbmc LANGUAGES C CXX)


target_sources(app PRIVATE
  src/main.c
  src/sys_mmap_impl.c
  src/test_deployment.c
  src/test_sd_event.c
  # ${CMAKE_CURRENT_LIST_DIR}/../../samples/test_event/sd_event_test.c
  src/thread_dependency_mgr.cpp
  src/task_def.cpp
  src/misc.c
  src/net_stack_cfg.c
  src/dbroker.c
  src/socketpool.c
)

# Handle Unix socket compatibility
target_compile_definitions(app PRIVATE
-D__ZEPHYR__
-D_POSIX_SOCKETS
-D_AF_UNIX_EXISTS_=1
)

zephyr_compile_options(-msmall-data-limit=0 -Wno-strict-prototypes -Wno-macro-redefinition)
target_link_options(app PRIVATE -Wl,--no-relax)

if (CONFIG_ZEPHYRBMC_FILESYSTEM)
zephyr_library_link_libraries(filesystem)
endif()

zephyr_library_link_libraries(
	dbus-broker
	basu
)

# zephyr_library_link_libraries(sd-bus)
zephyr_library_link_libraries(zephyr-osal)
# zephyr_library_link_libraries(boost)
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
)
