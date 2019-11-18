cmake_minimum_required(VERSION 3.0)

# Nom du projet
project(CPCCore)

set (USE_SHARED_CRT OFF)
set (gtest_force_shared_crt OFF)

set (SKIP_INSTALL_ALL ON)
set (BENCHMARK_ENABLE_GTEST_TESTS OFF)
set (BENCHMARK_ENABLE_TESTING OFF)

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

set (CPCCORE			${PROJECT_SOURCE_DIR}/CPCCoreEmu 		CACHE STRING "Dossier CPC Core Emu"		FORCE)

add_subdirectory(CPCCoreEmu)
add_subdirectory(googletest/googletest)
add_subdirectory(zlib)
add_subdirectory(UnitTests)
add_subdirectory(benchmark)
add_subdirectory(SugarboxBenchmarks)



