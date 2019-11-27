cmake_minimum_required(VERSION 3.0)

project(SugarboxBenchmarks)

include_directories( PUBLIC ${CPCCORE} ${googlebenchmark_SOURCE_DIR})

set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
		_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING 
        )

set(CMAKE_CXX_STANDARD 17)
 
IF(MSVC)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")  
ENDIF(MSVC)

if (USE_SHARED_CRT)
else()
   foreach(CompilerFlag ${CompilerFlags})
     string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
   endforeach()
endif()

add_executable( SugarboxBenchmarks
            benchmark.cpp
            TestUtils.cpp
            ../UnitTests/Display.cpp
)

# Link test executable against gtest & gtest_main
target_link_libraries(SugarboxBenchmarks zlibstatic benchmark CPCCoreEmu sfml-main sfml-graphics)
