find_program(RARS_CC NAMES clang REQUIRED)
find_program(AWK NAMES gawk awk REQUIRED)

set(RARS_JAR "${CMAKE_SOURCE_DIR}/../../materials/rars/rars1_6.jar"
    CACHE FILEPATH "Путь к rars1_6.jar")

set(RARS_MAX_STEPS 20000000 CACHE STRING "Предел шагов симуляции")

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/rars/prefs)
find_program(RARS_JAVA java)
if(NOT RARS_JAVA)

    find_program(NIX nix)
    if(NIX)
        execute_process(
            COMMAND ${NIX} build --no-link --print-out-paths "nixpkgs#jre"
            OUTPUT_VARIABLE jre
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE status
            ERROR_QUIET)
        if(status EQUAL 0 AND EXISTS "${jre}/bin/java")
            set(RARS_JAVA "${jre}/bin/java" CACHE FILEPATH "Java для RARS" FORCE)
        endif()
    endif()
endif()

set(rars_flags
    --target=riscv32 -march=rv32imf -mabi=ilp32f -Os
    ${core_flags}

    -finline-hint-functions
    -ffixed-x31
    -mllvm -enable-global-merge=false
    -fno-addrsig -fno-ident -fno-asynchronous-unwind-tables)

function(rars_image name)
    set(dir ${CMAKE_BINARY_DIR}/rars/${name})
    set(asm_files "")

    foreach(src ${ARGN})
        get_filename_component(base ${src} NAME_WE)
        get_filename_component(full ${src} ABSOLUTE BASE_DIR ${CMAKE_SOURCE_DIR})
        set(asm ${dir}/${base}.s)

        add_custom_command(
            OUTPUT ${asm}
            COMMAND ${CMAKE_COMMAND}
                -DCC=${RARS_CC}
                "-DFLAGS=${rars_flags}"
                -DINCDIR=${CMAKE_SOURCE_DIR}/headers
                -DSRC=${full}
                -DGAS=${dir}/${base}.gas
                -DASM=${asm}
                -DAWK=${AWK}
                -DFILTER=${CMAKE_SOURCE_DIR}/cmake/gas2rars.awk
                -P ${CMAKE_SOURCE_DIR}/cmake/rars_compile.cmake
            DEPENDS ${full} ${CMAKE_SOURCE_DIR}/headers/basic.h
                    ${CMAKE_SOURCE_DIR}/cmake/gas2rars.awk

            VERBATIM
            COMMENT "rv32 ${name}/${base}.s")

        list(APPEND asm_files ${asm})
    endforeach()

    set(port ${dir}/port_rars.s)
    add_custom_command(
        OUTPUT ${port}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/src/port/rars.s ${port}
        DEPENDS ${CMAKE_SOURCE_DIR}/src/port/rars.s)
    list(APPEND asm_files ${port})

    add_custom_target(asm_${name} ALL DEPENDS ${asm_files})
    set(${name}_asm ${asm_files} PARENT_SCOPE)
endfunction()

function(rars_launcher out)
    set(${out}
        ${RARS_JAVA} -Djava.util.prefs.userRoot=${CMAKE_BINARY_DIR}/rars/prefs -jar ${RARS_JAR}
        nc me sm we se1 ae2 ${RARS_MAX_STEPS}
        ${ARGN}
        pa
        PARENT_SCOPE)
endfunction()

rars_image(basic ${core_sources} ${core_entry})
