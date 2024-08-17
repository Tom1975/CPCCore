cmake_minimum_required(VERSION 3.0)

project(unitTests)

include(GoogleTest)

enable_testing()

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} PUBLIC ${CPCCORE} ${PROJECT_SOURCE_DIR}/../SFML/include)

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

add_executable( unitTests  
						   ASIC.cpp
                     Display.cpp 
                     Display.h 
						   PLUS.cpp
						   Test.cpp 
                     TestAutorun.cpp 
                     TestContainers.cpp 
                     Test_Dumps.cpp 
						   Test_Dumps_tape.cpp 
                     TestFDC.cpp 
						   TestSettings.cpp
                     TestZ80.cpp 
						   TestUtils.h
						   TestUtils.cpp
                     TestZ84C30.cpp

)

gtest_discover_tests(  unitTests
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
target_link_libraries(unitTests PUBLIC zlibstatic gtest gtest_main CPCCoreEmu sfml-graphics ${LIBZA} )

set (WORKING_DIR_TEST "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>")
MESSAGE ( STATUS "Unit test working dir = ${WORKING_DIR_TEST}")
MESSAGE ( STATUS "Unit test Current binary dir = ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>")

add_test( NAME unitTests COMMAND unitTests WORKING_DIRECTORY ${WORKING_DIR_TEST})

install(TARGETS unitTests 
            RUNTIME DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>"
            LIBRARY DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>"
            ARCHIVE DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>")

install ( FILES "./TestConf.ini" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>) 
install ( FILES "./TestConf_0.ini" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)  
install ( FILES "./TestConf_1.ini" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)  
install ( FILES "./TestConf_2.ini" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)  
install ( FILES "./TestConf_3.ini" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)  
install ( FILES "./TestConf_4.ini" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)  

install ( DIRECTORY "./ROM" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>) 
install ( DIRECTORY "./CART" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)  
install ( DIRECTORY "./CONF" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)  
install ( DIRECTORY "./res" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>) 
