cbench_module(math math.c)

add_executable(dhry dhry_1.c dhry_2.c)
set_target_properties(dhry PROPERTIES COMPILE_FLAGS "-Wno-implicit-function-declaration -Wno-return-type -Wno-implicit-int")
add_executable(linpack linpacknew.c)
set_target_properties(linpack PROPERTIES COMPILE_FLAGS "-Wno-unused-variable")
add_executable(whetstone whetstone.c)
set_target_properties(whetstone PROPERTIES COMPILE_FLAGS "-Wno-unused-variable -Wno-unused-label -Wno-unused-but-set-variable")
target_link_libraries(whetstone m)

install(TARGETS whetstone linpack DESTINATION lib/cbenchsuite/math/)
install(FILES sqrt.dc DESTINATION lib/cbenchsuite/math/)
