cbench_module(linux_perf
	perf.c
	perf_hackbench.c
	sched-pipe.c
)

find_package(Threads)
add_executable(hackbench hackbench.c)
target_link_libraries(hackbench ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS hackbench DESTINATION lib/cbenchsuite/linux_perf)
