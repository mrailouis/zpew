include(ZpewCoreDependencies)

add_library(zpew_core STATIC
    src/data/gap.cpp
    src/data/iso_date.cpp
    src/data/manifest.cpp
    src/data/snapshot_csv.cpp
    src/data/snapshot_data_source.cpp
    src/data/trading_calendar.cpp
    src/data/validation.cpp
    src/io/binary_file.cpp
    src/io/sha256.cpp
    src/models/ewma_model.cpp
    src/models/realised_vol_model.cpp
    src/optimiser/nelder_mead.cpp
    src/returns/log_returns.cpp
    src/returns/moving_average.cpp
    src/returns/realised_variance.cpp
    src/returns/rolling_mean.cpp
    src/version.cpp)
add_library(zpew::core ALIAS zpew_core)

target_include_directories(zpew_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(zpew_core PUBLIC cxx_std_20)
target_link_libraries(zpew_core PRIVATE nlohmann_json::nlohmann_json)
target_compile_definitions(zpew_core PRIVATE ZPEW_VERSION="${PROJECT_VERSION}")

zpew_configure_target(zpew_core)
