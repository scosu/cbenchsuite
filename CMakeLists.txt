cmake_minimum_required(VERSION 2.6)

project(cbenchsuite)

set(cbenchsuite_VERSION_MAJOR 1)
set(cbenchsuite_VERSION_MINOR 0)

set(DB_DIR "~/.cbenchsuite/db/" CACHE STRING "Directory of the cbenchsuite database")
set(MODULE_DIR ${CMAKE_INSTALL_PREFIX}/lib/cbenchsuite_modules/ CACHE STRING "Location of modules after installing")
set(WORK_DIR "~/.cbenchsuite/workdir/" CACHE STRING "Work directory of cbenchsuite")
set(DOWNLOAD_DIR "~/.cache/cbenchsuite/downloads/" CACHE STRING "Download directory of cbenchsuite")

set(CONTROLLER_PRIORITY "-20" CACHE STRING "Priority of the cbenchsuite controller thread")
set(EXECUTION_PRIORITY 0 CACHE STRING "Priority of the benchmark execution thread")
set(MONITOR_PRIORITY "-15" CACHE STRING "Priority of the monitor thread")
set(LOG_LEVEL 6 CACHE STRING "Default log level of cbenchsuite 7 is debug, 6 is info, etc.")

set(DEFAULT_WARMUP_RUNS 2 CACHE STRING "Default number of warmup runs")
set(DEFAULT_STDERR_PERCENT "2.0" CACHE STRING "Default percentage of standard error that should be reached")
set(DEFAULT_MIN_RUNTIME 300 CACHE STRING "Default minimum runtime per benchmark")
set(DEFAULT_MAX_RUNTIME 1800 CACHE STRING "Default maximum runtime per benchmark")
set(DEFAULT_MIN_RUNS 3 CACHE STRING "Default minimum runs per benchmark")
set(DEFAULT_MAX_RUNS 1000 CACHE STRING "Default maximum runs per benchmark")

configure_file(${PROJECT_SOURCE_DIR}/cbench.h.in ${PROJECT_BINARY_DIR}/cbench_config.h)

find_library(SQLITE_LIBRARY NAMES sqlite3)
if (NOT SQLITE_LIBRARY)
	message(FATAL_ERROR "Failed to find library libsqlite3")
endif (NOT SQLITE_LIBRARY)

find_library(UUID_LIBRARY NAMES uuid)
if (NOT UUID_LIBRARY)
	message(FATAL_ERROR "Failed to find library libuuid")
endif (NOT UUID_LIBRARY)

find_package(Threads)

include_directories(${PROJECT_BINARY_DIR} include libs/klib/include libs)
add_subdirectory(src)
add_subdirectory(modules)
set(CBENCHSUITE_SOURCES ${CBENCHSUITE_SOURCES} libs/klib/printk.c libs/sha256-gpl/sha256.c)

add_definitions(-D_GNU_SOURCE)
add_executable(cbenchsuite ${CBENCHSUITE_SOURCES})
target_link_libraries(cbenchsuite ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(cbenchsuite dl)
target_link_libraries(cbenchsuite m)
target_link_libraries(cbenchsuite sqlite3)
target_link_libraries(cbenchsuite uuid)
install(TARGETS cbenchsuite DESTINATION bin)

set(CBENCH_TARGETS ${CBENCHSUITE_MODULES} CACHE STRING "bla")
