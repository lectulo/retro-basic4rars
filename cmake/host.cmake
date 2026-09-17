add_library(host_core ${core_sources})
target_include_directories(host_core PUBLIC headers)
target_compile_options(host_core PUBLIC ${core_flags} -g)

set(host_port src/port/host.c)

add_executable(basic ${core_entry} ${host_port})
target_link_libraries(basic host_core)
