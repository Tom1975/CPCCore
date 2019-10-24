cmake_minimum_required(VERSION 3.0)

# Nom du projet
project(CPCCore)

IF(MSVC)
   set (USE_SHARED_CRT OFF CACHE BOOL "Use Shared CRT" FORCE )
endif()


set (CMAKE_INSTALL_PREFIX ${SugarConvDsk_BINARY_DIR}/Install)
set (SKIP_INSTALL_ALL ON)

add_subdirectory(zlib)
add_subdirectory(CPCCoreEmu)


