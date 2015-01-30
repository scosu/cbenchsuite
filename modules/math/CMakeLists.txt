cbench_module(math math.c)

add_executable(dhry dhry_1.c dhry_2.c)
add_executable(linpack linpacknew.c)
add_executable(whetstone whetstone.c)
target_link_libraries(whetstone m)

install(TARGETS whetstone linpack DESTINATION lib/cbenchsuite_modules/math/)
install(FILES sqrt.dc DESTINATION lib/cbenchsuite_modules/math/)
