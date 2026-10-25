# The documentation check, after the comment template of the CGVR programming guideline (Universitaet Bremen,
# https://cgvr.informatik.uni-bremen.de/progr/guidelines.shtml; docs/STYLE.md): every file, class, function, variable,
# typedef and macro of src/ and tests/ -- private members, static functions and anonymous namespaces included -- has its
# documentation, every parameter its direction ([in], [out], [in,out]), every return value its description, and
# Doxygen finds no documentation error. The test `doccheck` (ctest) fails and names what lacks it.
#
#   include(cmake/Doccheck.cmake) then sunstreak_doccheck() where enable_testing() is in effect, or as a script:
#   cmake -DDOCCHECK=1 -DDOXYGEN=<doxygen> -DROOT=<source dir> -DOUT=<work dir> -P Doccheck.cmake

if(CMAKE_SCRIPT_MODE_FILE AND DOCCHECK)
    set(inputs "")
    set(globs "")
    foreach(d src tests)
        if(IS_DIRECTORY "${ROOT}/${d}")
            string(APPEND inputs " \"${ROOT}/${d}\"")
            list(APPEND globs "${ROOT}/${d}/*.h" "${ROOT}/${d}/*.cpp")
        endif()
    endforeach()
    file(MAKE_DIRECTORY "${OUT}")
    set(log "${OUT}/undocumented.txt")
    file(REMOVE "${log}")
    file(WRITE "${OUT}/Doxyfile" "PROJECT_NAME = \"Sunstreak\"
OUTPUT_DIRECTORY = \"${OUT}\"
INPUT = ${inputs}
FILE_PATTERNS = *.h *.cpp
RECURSIVE = YES
EXTRACT_ALL = NO
EXTRACT_PRIVATE = YES
EXTRACT_STATIC = YES
EXTRACT_LOCAL_CLASSES = YES
EXTRACT_ANON_NSPACES = YES
EXTRACT_LOCAL_METHODS = YES
DISTRIBUTE_GROUP_DOC = YES
WARN_IF_UNDOCUMENTED = YES
WARN_IF_DOC_ERROR = YES
WARN_NO_PARAMDOC = YES
WARN_IF_INCOMPLETE_DOC = YES
WARN_LOGFILE = \"${log}\"
GENERATE_HTML = NO
GENERATE_LATEX = NO
QUIET = YES
HAVE_DOT = NO
")
    execute_process(COMMAND "${DOXYGEN}" "${OUT}/Doxyfile" WORKING_DIRECTORY "${ROOT}" RESULT_VARIABLE r OUTPUT_QUIET ERROR_QUIET)
    if(NOT r EQUAL 0)
        message(FATAL_ERROR "doxygen failed (${r})")
    endif()
    # Every gap Doxygen reports: something undocumented, a parameter or a return value not described, a documentation
    # error (a @param of a name the function does not have, a placeholder taken for HTML) ...
    set(missing "")
    if(EXISTS "${log}")
        file(STRINGS "${log}" lines REGEX "warning:")
        set(missing ${lines})
    endif()
    # ... and every @param with its direction.
    file(GLOB_RECURSE sources ${globs})
    foreach(f ${sources})
        file(STRINGS "${f}" plain REGEX "[@\\\\]param[ \t]")
        foreach(l ${plain})
            list(APPEND missing "${f}: @param without a direction: ${l}")
        endforeach()
    endforeach()
    list(LENGTH missing n)
    if(n GREATER 0)
        list(SUBLIST missing 0 40 shown)
        string(REPLACE ";" "\n" shown "${shown}")
        message(FATAL_ERROR "${n} documentation gaps (Doxygen's in ${log}):\n${shown}")
    endif()
    message(STATUS "doccheck: everything documented")
    return()
endif()

set(SUNSTREAK_DOCCHECK_FILE "${CMAKE_CURRENT_LIST_FILE}" CACHE INTERNAL "")

# sunstreak_doccheck(): the test `doccheck`, where Doxygen is found.
function(sunstreak_doccheck)
    find_program(SUNSTREAK_DOXYGEN doxygen PATHS "C:/Program Files/doxygen/bin")
    if(NOT SUNSTREAK_DOXYGEN)
        message(STATUS "doccheck: no doxygen, no test")
        return()
    endif()
    add_test(NAME doccheck COMMAND "${CMAKE_COMMAND}" -DDOCCHECK=1 "-DDOXYGEN=${SUNSTREAK_DOXYGEN}" "-DROOT=${CMAKE_SOURCE_DIR}"
                                   "-DOUT=${CMAKE_BINARY_DIR}/doccheck" -P "${SUNSTREAK_DOCCHECK_FILE}")
    set_tests_properties(doccheck PROPERTIES LABELS "docs")
endfunction()
