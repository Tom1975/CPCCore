cmake_minimum_required(VERSION 3.0)

# Nom du projet
project(CPCCore)

set(GENERATE_BENCHMARK OFF CACHE BOOL "Generate SugarBenchmark")
set(GENERATE_UNITTESTS OFF CACHE BOOL "Generate UnitTests")

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

foreach(CompilerFlag ${CompilerFlags})
  string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()

set (USE_SHARED_CRT OFF BOOL)
set (gtest_force_shared_crt OFF BOOL)

set (SKIP_INSTALL_ALL ON BOOL)
set (BENCHMARK_ENABLE_GTEST_TESTS OFF BOOL)
set (BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "ENABLE tests")
set (BENCHMARK_ENABLE_INSTALL OFF BOOL)

set (CPCCORE			${PROJECT_SOURCE_DIR}/CPCCoreEmu 		CACHE STRING "Dossier CPC Core Emu"		FORCE)

add_subdirectory(zlib)
add_subdirectory(CPCCoreEmu)

if (GENERATE_UNITTESTS)
   add_subdirectory(googletest/googletest)
   add_subdirectory(UnitTests)
   add_subdirectory(SFML)
endif()

if (GENERATE_BENCHMARK)
   add_subdirectory(benchmark)
   add_subdirectory(SugarboxBenchmarks)
endif()


