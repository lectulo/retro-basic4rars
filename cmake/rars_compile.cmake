get_filename_component(outdir ${ASM} DIRECTORY)
file(MAKE_DIRECTORY ${outdir})

execute_process(
    COMMAND ${CC} ${FLAGS} -I ${INCDIR} -S ${SRC} -o ${GAS}
    RESULT_VARIABLE status
    ERROR_VARIABLE diagnostics)

if(NOT status EQUAL 0)
    message(FATAL_ERROR "не собралось под riscv32: ${SRC}\n${diagnostics}")
endif()

execute_process(
    COMMAND ${AWK} -f ${FILTER} ${GAS}
    OUTPUT_FILE ${ASM}
    RESULT_VARIABLE status
    ERROR_VARIABLE diagnostics)

if(NOT status EQUAL 0)
    file(REMOVE ${ASM})
    message(FATAL_ERROR "фильтр не пропустил ${GAS}\n${diagnostics}")
endif()
