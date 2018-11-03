cmake_minimum_required(VERSION 3.0)

# Nom du projet
project(CPCCore)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
                 ${CMAKE_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
  include_directories("${gtest_SOURCE_DIR}/include")
endif()


# Libs
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR} )
set (CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} 
						${PROJECT_SOURCE_DIR} )
						

# chemin
#set (CPCCORE			${PROJECT_SOURCE_DIR}/CPCCoreEmu 		CACHE string "Dossier CPC Core Emu"		FORCE)

# inclusion des dossiers
add_subdirectory(CPCCoreEmu)
add_subdirectory(UnitTests)

