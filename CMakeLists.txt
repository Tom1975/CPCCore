cmake_minimum_required(VERSION 3.0)

# Nom du projet
project(CPCCore)

set (gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set (CMAKE_INSTALL_PREFIX ${SugarConvDsk_BINARY_DIR}/Install)
set (SKIP_INSTALL_ALL ON)

add_subdirectory(zlib)
add_subdirectory(CPCCoreEmu)
add_subdirectory(googletest/googletest)
add_subdirectory(UnitTests)


