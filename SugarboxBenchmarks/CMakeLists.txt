cmake_minimum_required(VERSION 3.0)

project(SugarboxBenchmarks)
#
#MESSAGE ("googlebenchmark_SOURCE_DIR : "${googlebenchmark_SOURCE_DIR} )
include_directories( PUBLIC ${CPCCORE})

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

foreach(CompilerFlag ${CompilerFlags})
   string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()

add_executable( SugarboxBenchmarks
            benchmark.cpp
            Display.cpp
            TestUtils.cpp

)

# Link test executable against gtest & gtest_main
target_link_libraries(SugarboxBenchmarks zlibstatic CPCCoreEmu benchmark Shlwapi.lib Comctl32.lib XmlLite.lib 
                                             Msimg32.lib Winmm.lib )
