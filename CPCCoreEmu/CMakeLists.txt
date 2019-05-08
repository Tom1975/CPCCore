cmake_minimum_required(VERSION 3.0)
project(CPCCoreEmu)

file( GLOB SRCS
	"*.cpp"
	)


file( GLOB HEADERS
	"*.h"
)

#Fichier ressource
include_directories( ${CMAKE_CURRENT_BINARY_DIR}/../zlib)
add_library(CPCCoreEmu ${SRCS} ${HEADERS})
target_include_directories( CPCCoreEmu PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../zlib)

set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        )

IF(MSVC)
   set_property(TARGET CPCCoreEmu APPEND PROPERTY LINK_FLAGS /DEBUG)
   ADD_DEFINITIONS(/arch:SSE2)
   set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Ot /Ob2 /GL /Gs- /GS- /Gy /Oi /DVERSION=${VERSION} /DBUILD_NUMBER=${BUILD_NUMBER}")
   set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ot /Ob2 /GL /Gs- /GS- /Gy /Oi /DVERSION=${VERSION} /DBUILD_NUMBER=${BUILD_NUMBER}")
ENDIF(MSVC)

if (USE_SHARED_CRT)
else()
   foreach(CompilerFlag ${CompilerFlags})
     string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
   endforeach()
endif()

if(CMAKE_COMPILER_IS_GNUCC)
	set ( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} --std=c++1z )
endif()

