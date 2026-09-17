add_executable(unit unit.c ${CMAKE_SOURCE_DIR}/src/port/host.c)
target_link_libraries(unit host_core)
add_test(NAME unit_host COMMAND unit)

rars_image(unit ${core_sources} ${CMAKE_CURRENT_SOURCE_DIR}/unit.c)

set(test_targets host)
set(host_command $<TARGET_FILE:basic>)
if(RARS_JAVA AND EXISTS "${RARS_JAR}")
    list(APPEND test_targets rars)
    rars_launcher(rars_command ${basic_asm})
    rars_launcher(unit_command ${unit_asm})
    add_test(NAME unit_rars COMMAND ${unit_command})
endif()

foreach(target ${test_targets})
    foreach(name quadratic func_table sort integrate game text)
        add_test(NAME integration_${target}_${name}
            COMMAND ${CMAKE_COMMAND}
                "-DLAUNCH=${${target}_command}"
                -DDIR=${CMAKE_CURRENT_SOURCE_DIR}/progs
                -DNAME=demo_${name}
                -P ${CMAKE_SOURCE_DIR}/cmake/run_integration.cmake)
    endforeach()
endforeach()
