include_guard(GLOBAL)

include(FetchContent)

set(JSON_Install OFF CACHE BOOL "" FORCE)

FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    SYSTEM)
FetchContent_MakeAvailable(nlohmann_json)
