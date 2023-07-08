cmake_minimum_required(VERSION 3.0)
project(AmstradCore)

file( GLOB SRCS
	"*.cpp" 
	)

file( GLOB HEADERS
	"*.h" "*.hpp"
)

#Fichier ressource
MESSAGE ("CMAKE_CURRENT_BINARY_DIR =" ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${INSTALL_INC_DIR}  ${CMAKE_CURRENT_BINARY_DIR}/../zlib)
add_library(AmstradCore ${SRCS} ${HEADERS})

set_target_properties(AmstradCore PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories( AmstradCore PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../zlib ${CMAKE_CURRENT_BINARY_DIR}/../zlib ${CMAKE_CURRENT_SOURCE_DIR})

set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        )

target_compile_features(AmstradCore PRIVATE cxx_std_17)


IF(MSVC)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")  
   set_property(TARGET AmstradCore APPEND PROPERTY LINK_FLAGS /DEBUG)
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

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set ( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} --std=c++1z )
endif()
