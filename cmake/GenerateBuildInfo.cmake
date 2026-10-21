set(commit "unknown")
set(dirty "true")
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY ${SOURCE_DIR}
        OUTPUT_VARIABLE git_commit
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE git_commit_result
        ERROR_QUIET)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} status --porcelain --untracked-files=no
        WORKING_DIRECTORY ${SOURCE_DIR}
        OUTPUT_VARIABLE git_status
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE git_status_result
        ERROR_QUIET)
    if(git_commit_result EQUAL 0)
        set(commit "${git_commit}")
    endif()
    if(git_status_result EQUAL 0 AND git_status STREQUAL "")
        set(dirty "false")
    endif()
endif()

set(content "#include \"build_info.hpp\"

namespace zpew::cli {

BuildInfo build_info() noexcept {
    return {.commit = \"${commit}\", .dirty = ${dirty}, .compiler = \"${COMPILER}\"};
}

}
")

if(EXISTS "${OUTPUT}")
    file(READ "${OUTPUT}" existing)
else()
    set(existing "")
endif()
if(NOT existing STREQUAL content)
    file(WRITE "${OUTPUT}" "${content}")
endif()
