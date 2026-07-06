cmake_minimum_required(VERSION 3.16...3.28)

project(SugarboxBenchmarks)

include_directories(${CPCCORE} ${googlebenchmark_SOURCE_DIR})

add_executable(SugarboxBenchmarks
    benchmark.cpp
    TestUtils.cpp
    ../UnitTests/Display.cpp
)

target_compile_features(SugarboxBenchmarks PRIVATE cxx_std_17)

if(USE_SHARED_CRT)
else()
    set(CompilerFlags
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
    )
    foreach(CompilerFlag ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()
endif()

target_link_libraries(SugarboxBenchmarks zlibstatic benchmark CPCCoreEmu sfml-main sfml-graphics)
