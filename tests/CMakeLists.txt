include(FetchContent)

FetchContent_Declare(Catch2
    URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.16.0.tar.gz
    URL_HASH SHA256=0957cae5821b17ce07f0833aaa52b5137643a8382203221f363a8303c109af34
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    SYSTEM)
FetchContent_MakeAvailable(Catch2)
set_target_properties(Catch2 Catch2WithMain PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
set_target_properties(Catch2 Catch2WithMain PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)

add_executable(zpew_tests
    version_test.cpp)

target_link_libraries(zpew_tests PRIVATE zpew::core Catch2::Catch2WithMain)
target_compile_definitions(zpew_tests PRIVATE ZPEW_EXPECTED_VERSION="${PROJECT_VERSION}")

zpew_configure_target(zpew_tests)

list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
include(Catch)
catch_discover_tests(zpew_tests)

if(ZPEW_BUILD_CLI)
    add_test(NAME cli.version COMMAND zpew_cli --version)
    set_tests_properties(cli.version PROPERTIES
        PASS_REGULAR_EXPRESSION "^zpew-cli ${PROJECT_VERSION}\n$")

    add_test(NAME cli.rejects_unknown_arguments COMMAND zpew_cli --bogus)
    set_tests_properties(cli.rejects_unknown_arguments PROPERTIES WILL_FAIL TRUE)
endif()
