find_program(CPPCHECK_EXE "cppcheck")

if(CPPCHECK_EXE)
    message(STATUS "[CPPCHECK] at ${CPPCHECK_EXE}")
    add_custom_target(cppcheck)

    add_custom_command(TARGET cppcheck POST_BUILD
        COMMAND ${CPPCHECK_EXE} --template=gcc -f --std=posix --std=c99 --std=c++11 --enable=all -i ${PROJECT_BINARY_DIR} -I ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}
    )
else()
    message(STATUS "[CPPCHECK] Could not find cppcheck executable")
endif()
