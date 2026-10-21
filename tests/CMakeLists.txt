include(FetchContent)

FetchContent_Declare(Catch2
    URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.16.0.tar.gz
    URL_HASH SHA256=0957cae5821b17ce07f0833aaa52b5137643a8382203221f363a8303c109af34
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    SYSTEM)
FetchContent_MakeAvailable(Catch2)
set_target_properties(Catch2 Catch2WithMain PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
zpew_configure_third_party_target(Catch2)
zpew_configure_third_party_target(Catch2WithMain)

add_executable(zpew_tests
    data/gap_test.cpp
    data/iso_date_test.cpp
    data/manifest_test.cpp
    data/snapshot_csv_test.cpp
    data/snapshot_data_source_test.cpp
    data/trading_calendar_test.cpp
    data/validation_test.cpp
    error_test.cpp
    io/binary_file_test.cpp
    io/sha256_test.cpp
    models/ewma_model_test.cpp
    models/realised_vol_model_test.cpp
    models/vol_model_contract_test.cpp
    returns/log_returns_test.cpp
    returns/moving_average_test.cpp
    returns/realised_variance_test.cpp
    returns/rolling_mean_test.cpp
    version_test.cpp)

target_link_libraries(zpew_tests PRIVATE zpew::core Catch2::Catch2WithMain nlohmann_json::nlohmann_json)
target_include_directories(zpew_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
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

    set(zpew_cli_fixtures ${CMAKE_CURRENT_SOURCE_DIR}/cli/fixtures)

    add_test(NAME cli.check_data_accepts_valid_snapshots
        COMMAND zpew_cli check-data --manifest ${zpew_cli_fixtures}/valid/manifest.json)
    set_tests_properties(cli.check_data_accepts_valid_snapshots PROPERTIES
        PASS_REGULAR_EXPRESSION "ok    TEST +3 bars  2024-01-04 to 2024-01-11  1 gap, largest 3 sessions after 2024-01-05\n1 of 1 snapshots verified")

    add_test(NAME cli.check_data_rejects_tampered_snapshots
        COMMAND zpew_cli check-data --manifest ${zpew_cli_fixtures}/tampered/manifest.json)
    set_tests_properties(cli.check_data_rejects_tampered_snapshots PROPERTIES WILL_FAIL TRUE)

    add_test(NAME cli.check_data_reports_tampering
        COMMAND zpew_cli check-data --manifest ${zpew_cli_fixtures}/tampered/manifest.json)
    set_tests_properties(cli.check_data_reports_tampering PROPERTIES
        PASS_REGULAR_EXPRESSION "FAIL  TEST +.*sha256 mismatch for TEST")

    add_test(NAME cli.forecast_prints_one_step_ahead_ewma
        COMMAND zpew_cli forecast --symbol TEST --model ewma --lambda 0.5 --seed 1
                --manifest ${zpew_cli_fixtures}/valid/manifest.json)
    set_tests_properties(cli.forecast_prints_one_step_ahead_ewma PROPERTIES
        PASS_REGULAR_EXPRESSION "# symbol=TEST model=EWMA\\(lambda=0.5, seed=1\\)\ndate,return,forecast_variance\n2024-01-11,0\\.00975617494536[0-9]*,9\\.706774520[0-9]*e-05\n$")

    add_test(NAME cli.forecast_rejects_unknown_models
        COMMAND zpew_cli forecast --symbol TEST --model garch
                --manifest ${zpew_cli_fixtures}/valid/manifest.json)
    set_tests_properties(cli.forecast_rejects_unknown_models PROPERTIES WILL_FAIL TRUE)

    add_test(NAME cli.forecast_rejects_unknown_options
        COMMAND zpew_cli forecast --symbol TEST --model ewma --alpha 0.1)
    set_tests_properties(cli.forecast_rejects_unknown_options PROPERTIES WILL_FAIL TRUE)

    add_test(NAME cli.check_data_fails_on_missing_manifest
        COMMAND zpew_cli check-data --manifest ${zpew_cli_fixtures}/missing/manifest.json)
    set_tests_properties(cli.check_data_fails_on_missing_manifest PROPERTIES WILL_FAIL TRUE)
endif()
