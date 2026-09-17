set(bas "${DIR}/${NAME}.bas")
set(expected "${DIR}/${NAME}.expected")
set(input "${DIR}/${NAME}.in")

set(args "")
if(EXISTS "${bas}")
    set(args "${bas}")
endif()

if(NOT EXISTS "${expected}")
    message(FATAL_ERROR "нет эталона: ${expected}")
endif()

set(redirect "")
if(EXISTS "${input}")
    set(redirect INPUT_FILE "${input}")
endif()

set(want_status 0)
if(EXISTS "${DIR}/${NAME}.code")
    file(READ "${DIR}/${NAME}.code" want_status)
    string(STRIP "${want_status}" want_status)
endif()

execute_process(
    COMMAND ${LAUNCH} ${args}
    TIMEOUT 30
    ${redirect}
    OUTPUT_VARIABLE actual
    ERROR_VARIABLE diagnostics
    RESULT_VARIABLE status)

file(READ "${expected}" want)

if(NOT status STREQUAL want_status)
    message(FATAL_ERROR
        "код возврата ${status}, ожидался ${want_status} (${NAME})\n"
        "--- вывод ---\n${actual}"
        "--- stderr ---\n${diagnostics}")
endif()

if(NOT actual STREQUAL want)
    message(FATAL_ERROR
        "вывод не совпал с эталоном ${NAME}.expected\n"
        "--- получено ---\n${actual}"
        "--- ожидалось ---\n${want}"
        "--- stderr ---\n${diagnostics}"
        "--- код возврата: ${status} ---")
endif()
