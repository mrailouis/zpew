include_guard(GLOBAL)

add_library(zpew_options INTERFACE)
add_library(zpew::options ALIAS zpew_options)

target_compile_features(zpew_options INTERFACE cxx_std_20)

target_compile_options(zpew_options INTERFACE
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wpedantic -Werror -ffp-contract=off>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /permissive- /utf-8 /Zc:__cplusplus>)

target_compile_definitions(zpew_options INTERFACE
    $<$<CONFIG:Debug>:_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG>
    $<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>)

add_library(zpew_sanitizers INTERFACE)
add_library(zpew::sanitizers ALIAS zpew_sanitizers)
target_link_libraries(zpew_options INTERFACE zpew::sanitizers)

if(ZPEW_ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        set(zpew_sanitizer_flags -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
        target_compile_options(zpew_sanitizers INTERFACE ${zpew_sanitizer_flags})
        target_link_options(zpew_sanitizers INTERFACE ${zpew_sanitizer_flags})
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(zpew_sanitizers INTERFACE /fsanitize=address)
    else()
        message(FATAL_ERROR "ZPEW_ENABLE_SANITIZERS is not supported for ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()

if(ZPEW_ENABLE_CLANG_TIDY)
    find_program(ZPEW_CLANG_TIDY_EXE NAMES clang-tidy REQUIRED)
endif()

function(zpew_configure_third_party_target target)
    target_link_libraries(${target} PRIVATE zpew::sanitizers)
endfunction()

function(zpew_configure_target target)
    target_link_libraries(${target} PRIVATE zpew::options)
    if(ZPEW_ENABLE_CLANG_TIDY)
        set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY "${ZPEW_CLANG_TIDY_EXE}")
    endif()
endfunction()
