cmake_minimum_required(VERSION 3.16)

project(unitTests)

include(GoogleTest)

enable_testing()

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} PUBLIC ${CPCCORE} ${PROJECT_SOURCE_DIR}/../SFML/include)

IF(MSVC)
	add_definitions(-DSAFESEH=NO)
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

set ( TEST_CONF
   TestConf.ini
   TestConf_0.ini
   TestConf_1.ini
   TestConf_2.ini
   TestConf_3.ini
   TestConf_4.ini
)
source_group("Test configurations" FILES ${TEST_CONF} )

add_executable( unitTests  
                  ${TEST_CONF}
						   ASIC.cpp
                     Display.cpp 
                     Display.h 
						   PLUS.cpp
						   Test.cpp 
                     TestAutorun.cpp 
                     TestContainers.cpp 
                     TestCprLoader.cpp
                     Test_Dumps.cpp 
						   Test_Dumps_tape.cpp
						   TestTapeRecording.cpp
						   TestTapeFluxExact.cpp
						   TestTapeOverdubByteExact.cpp
                     TestFDC.cpp
                     TestGx4000Keyboard.cpp
                     TestGx4000PortB.cpp
						   TestSettings.cpp
                     TestZ80.cpp 
						   TestUtils.h
						   TestUtils.cpp
						   TestWorkspace.h
						   TestWorkspace.cpp
						   TestWorkspaceIsolation.cpp
                     TestZ84C30.cpp
                     TestSnapshot.cpp
                     TestSnapshotMachineType.cpp
                     TestStateDeterminism.cpp
                     TestMachineState.cpp
                     TestMachineStateRam.cpp
                     TestMachineStateSize.cpp
                     TestMachineStateExpansions.cpp

)


set(CMAKE_CXX_STANDARD 17)
 
IF(MSVC)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")  
ENDIF(MSVC)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++17")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(LIBZA libz.a)
    elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
        set(LIBZA libz.a stdc++fs)
    else()
        set(LIBZA libz.a)
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

# Same directory as the single-binary test below: that is where the fixtures
# are installed. Discovered tests used to start in "${PROJECT_BINARY_DIR}/..",
# which holds no ROM/ or res/, and a missing ROM leaves the CPU with nothing to
# run rather than failing, so every test spun to its cycle budget.
# The engine reads its ROMs, keyboard maps and machine settings from one base
# directory, and they do not all live in one place in the source tree: the
# keyboard maps belong to CPCCoreEmu. Assemble the small ones (about 2 MB) next
# to the binary. res/ is 347 MB and stays where it is, reached through the
# search path below.
set(FIXTURE_DIR "${CMAKE_CURRENT_BINARY_DIR}/fixtures")
add_custom_command(TARGET unitTests POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/ROM"  "${FIXTURE_DIR}/ROM"
   COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/CONF" "${FIXTURE_DIR}/CONF"
   COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/CART" "${FIXTURE_DIR}/CART"
   COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/../CPCCoreEmu/Keyboards" "${FIXTURE_DIR}/Keyboards"
   COMMENT "Assembling test fixtures in ${FIXTURE_DIR}"
)

gtest_discover_tests(  unitTests
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                PROPERTIES TIMEOUT 600
                          ENVIRONMENT "CPCCORE_TEST_PATHS=FIXTURES=${FIXTURE_DIR}|FIXTURES_EXTRA=${CMAKE_CURRENT_SOURCE_DIR}|WORK=${CMAKE_CURRENT_BINARY_DIR}/test-work"
)

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
install ( DIRECTORY "../CPCCoreEmu/Keyboards" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>) 
