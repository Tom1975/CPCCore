cmake_minimum_required(VERSION 3.0)

project(unitTests)

include(GoogleTest)

enable_testing()

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} PUBLIC ${CPCCORE} ${PROJECT_SOURCE_DIR}/../SFML/include)

SET(CMAKE_CXX_FLAGS           "${CMAKE_CXX_FLAGS} /D \"SAFESEH:NO\"")
SET(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO")

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
target_link_libraries(unitTests PUBLIC "-SAFESEH:NO" zlibstatic gtest gtest_main CPCCoreEmu sfml-graphics ${LIBZA} )

add_test( NAME unitTests COMMAND unitTests)

install(TARGETS unitTests 
            RUNTIME DESTINATION "."
            LIBRARY DESTINATION "."
            ARCHIVE DESTINATION lib/static)

install ( FILES "./TestConf.ini" DESTINATION ${PROJECT_BINARY_DIR}/..) 
install ( DIRECTORY "./ROM" DESTINATION ${PROJECT_BINARY_DIR}/..) 
install ( DIRECTORY "./CART" DESTINATION ${PROJECT_BINARY_DIR}/..) 
install ( DIRECTORY "./CONF" DESTINATION ${PROJECT_BINARY_DIR}/..) 
install ( DIRECTORY "./res" DESTINATION ${PROJECT_BINARY_DIR}/..) 

# ini file
#install ( FILES "./TestConf.ini" CONFIGURATIONS Debug DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/ )
#install ( FILES "./TestConf.ini" CONFIGURATIONS Release DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/ )
#install ( FILES "./TestConf.ini" CONFIGURATIONS RelWithDebInfo DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/ )
#
## ROM & CONF
#install ( DIRECTORY "./ROM" CONFIGURATIONS Debug DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/ )
#install ( DIRECTORY "./ROM" CONFIGURATIONS Release DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/ )
#install ( DIRECTORY "./ROM" CONFIGURATIONS RelWithDebInfo DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/ )
#
## CARTs
#install ( DIRECTORY "./CART" CONFIGURATIONS Debug DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
#install ( DIRECTORY "./CART" CONFIGURATIONS Release DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
#install ( DIRECTORY "./CART" CONFIGURATIONS RelWithDebInfo DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/)
#
#install ( DIRECTORY "./CONF" CONFIGURATIONS Debug DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/ )
#install ( DIRECTORY "./CONF" CONFIGURATIONS Release DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/ )
#install ( DIRECTORY "./CONF" CONFIGURATIONS RelWithDebInfo DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/ )
## Res
#install ( DIRECTORY "./res" CONFIGURATIONS Debug DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/ )
#install ( DIRECTORY "./res" CONFIGURATIONS Release DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/ )
#install ( DIRECTORY "./res" CONFIGURATIONS RelWithDebInfo DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/ )

#zlib1.dll


#file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/res/After Burner" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/res" )
#file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/res/CONF" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
#file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/res/ROM" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}" )
#file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/res/z80" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/res" )
#file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/res/Dsk" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/res" )
#file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/res/Record" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/res" )
#
#configure_file("${CMAKE_CURRENT_SOURCE_DIR}/res/DEATHSWORD128K.DSK"                                               "${CMAKE_CURRENT_BINARY_DIR}/res/DEATHSWORD128K.DSK"  COPYONLY)
#configure_file("${CMAKE_CURRENT_SOURCE_DIR}/res/Discology60PlusA1 [MAXIT][SAMdisk36B19][Original].dsk"            "${CMAKE_CURRENT_BINARY_DIR}/res/Discology60PlusA1 [MAXIT][SAMdisk36B19][Original].dsk" COPYONLY)
#configure_file("${CMAKE_CURRENT_SOURCE_DIR}/res/30YMD double sides 1 and 2.hfe"                                   "${CMAKE_CURRENT_BINARY_DIR}/res/30YMD double sides 1 and 2.hfe" COPYONLY)
#configure_file("${CMAKE_CURRENT_SOURCE_DIR}/res/1942.raw"                                                         "${CMAKE_CURRENT_BINARY_DIR}/res/1942.raw" COPYONLY)
#configure_file("${CMAKE_CURRENT_SOURCE_DIR}/res/After Burner (UK) (1988) [Activision SEGA] (Pre-release).ipf"     "${CMAKE_CURRENT_BINARY_DIR}/res/After Burner (UK) (1988) [Activision SEGA] (Pre-release).ipf" COPYONLY)
#configure_file("${CMAKE_CURRENT_SOURCE_DIR}/res/The Demo [A].scp"                                                 "${CMAKE_CURRENT_BINARY_DIR}/res/The Demo [A].scp" COPYONLY)
#configure_file("${CMAKE_CURRENT_SOURCE_DIR}/res/necro.sfw"                                                        "${CMAKE_CURRENT_BINARY_DIR}/res/necro.sfw" COPYONLY)
#
#configure_file("${CMAKE_CURRENT_SOURCE_DIR}/res/Basil The Great Mouse Detective (UK) (1987) [Original] [TAPE].cdt" "${CMAKE_CURRENT_BINARY_DIR}/res/Basil The Great Mouse Detective (UK) (1987) [Original] [TAPE].cdt" COPYONLY)
#configure_file("${CMAKE_CURRENT_SOURCE_DIR}/res/Mask_1_1.csw"                                                      "${CMAKE_CURRENT_BINARY_DIR}/res/Mask_1_1.csw" COPYONLY)
#configure_file("${CMAKE_CURRENT_SOURCE_DIR}/res/Ultima Ratio (Firebird)(UK)(1987)[Original][TAPE].csw"             "${CMAKE_CURRENT_BINARY_DIR}/res/Ultima Ratio (Firebird)(UK)(1987)[Original][TAPE].csw" COPYONLY)
#configure_file("${CMAKE_CURRENT_SOURCE_DIR}/res/FootBallerOfTheYear2.wav"                                          "${CMAKE_CURRENT_BINARY_DIR}/res/FootBallerOfTheYear2.wav" COPYONLY)
#configure_file("${CMAKE_CURRENT_SOURCE_DIR}/res/K7_MICK.VOC"                                                       "${CMAKE_CURRENT_BINARY_DIR}/res/K7_MICK.VOC" COPYONLY)
#
#configure_file("${CMAKE_CURRENT_SOURCE_DIR}/res/TestConf.ini"                                                      "${CMAKE_CURRENT_BINARY_DIR}/TestConf.ini" COPYONLY)
