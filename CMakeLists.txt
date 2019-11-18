cmake_minimum_required(VERSION 3.0)

# Nom du projet
project(CPCCore)

set (USE_SHARED_CRT OFF)
set (gtest_force_shared_crt OFF)
set (SKIP_INSTALL_ALL ON)

set (CPCCORE			${PROJECT_SOURCE_DIR}/CPCCoreEmu 		CACHE STRING "Dossier CPC Core Emu"		FORCE)

add_subdirectory(CPCCoreEmu)
add_subdirectory(googletest/googletest)
add_subdirectory(zlib)
add_subdirectory(UnitTests)


