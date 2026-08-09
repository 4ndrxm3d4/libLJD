cmake_minimum_required(VERSION 3.22)
project(luajit-decompiler-v2 LANGUAGES CXX)

# ===============================================================
# Android-only shared library (.so)
# ===============================================================
add_library(ljd SHARED)

set_target_properties(ljd PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    CMAKE_CXX_FLAGS "-Wno-error"
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME "ljd"
)

target_include_directories(ljd PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/library/include
    ${CMAKE_CURRENT_SOURCE_DIR}/library/src
)

target_compile_definitions(ljd PRIVATE
    LJD_BUILD=1
    LJD_ANDROID=1
)

target_sources(ljd PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/bytecode/bytecode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bytecode/prototype.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ast/ast.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lua/lua.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/library/src/ljd_api.cpp
)

if(ANDROID)
    target_sources(ljd PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/library/src/ljd_jni.cpp
    )
    find_library(LOG_LIB log)
    target_link_libraries(ljd PRIVATE ${LOG_LIB})
endif()

# Hide all symbols by default; keep only the intended JNI/API exports visible.
set_target_properties(ljd PROPERTIES
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
)
