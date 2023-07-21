cmake_minimum_required(VERSION 3.0)

project(AmstradCoreTests)

include(GoogleTest)

enable_testing()

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} PUBLIC ${PROJECT_SOURCE_DIR}/../AmstradCore)

IF(MSVC)
	SET(CMAKE_CXX_FLAGS           "${CMAKE_CXX_FLAGS} /D \"SAFESEH:NO\"")
	SET(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
	SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
	SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO")
endif()

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

add_executable( AmstradCoreTests  
   Test_GateArray.cpp
   Test_Z80.cpp
)

gtest_discover_tests(  AmstradCoreTests
                WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/.."
)

set(CMAKE_CXX_STANDARD 17)
 
IF(MSVC)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")  
ENDIF(MSVC)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set ( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} --std=c++1z)
   if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      set (LIBZA libz.a )
   else()
      set (LIBZA libz.a stdc++fs)
   endif()
endif()

if (USE_SHARED_CRT)
else()
   foreach(CompilerFlag ${CompilerFlags})
     string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
   endforeach()
endif()

# Link test executable against gtest & gtest_main
target_link_libraries(AmstradCoreTests PUBLIC zlibstatic gtest gtest_main AmstradCore ${LIBZA} )

add_test( NAME AmstradCoreTests COMMAND AmstradCoreTests)

install(TARGETS AmstradCoreTests 
            RUNTIME DESTINATION "."
            LIBRARY DESTINATION "."
            ARCHIVE DESTINATION lib/static)
