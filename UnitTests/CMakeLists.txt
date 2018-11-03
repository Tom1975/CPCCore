cmake_minimum_required(VERSION 3.0)

project(UnitTests)
enable_testing()

include_directories( ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} ${CPCCoreEmu})

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


set(SRCS
	test.cpp
	)

set(HEADERS
)

add_executable( UnitTests   ${SRCS} ${HEADERS} )
include_directories(${CMAKE_CURRENT_LIST_DIR}/../CPCCoreEmu ${CMAKE_CURRENT_BINARY_DIR}/../zlib)

# Link test executable against gtest & gtest_main
target_include_directories( UnitTests PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../CPCCoreEmu ${CMAKE_CURRENT_LIST_DIR}/../zlib)
target_link_libraries(UnitTests gtest gtest_main CPCCoreEmu )

add_test( NAME UnitTests COMMAND UnitTests)

if(CMAKE_COMPILER_IS_GNUCC)
	set ( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} --std=c++1z )
endif()

set ( CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} )

if(MSVC)
	target_link_libraries ( UnitTests zlibstatic )
else ()
	target_link_libraries ( UnitTests  m stdc++fs libz.a libCPCCoreEmu.so)
endif()

if( MSVC)
	# Output temporary directory
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Debug/out)
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Release/out)
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/out)

	# Copy file to compare
	install ( DIRECTORY "./Expected" CONFIGURATIONS Debug DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/ )
	install ( DIRECTORY "./Expected" CONFIGURATIONS Release DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/ )
	install ( DIRECTORY "./Expected" CONFIGURATIONS RelWithDebInfo DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/ )

	install ( DIRECTORY "./Ressources" CONFIGURATIONS Debug DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/ )
	install ( DIRECTORY "./Ressources" CONFIGURATIONS Release DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/ )
	install ( DIRECTORY "./Ressources" CONFIGURATIONS RelWithDebInfo DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/ )
else()
	# Output temporary directory
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/out)

	# Copy file to compare
	install ( DIRECTORY "./Expected" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
	install ( DIRECTORY "./Ressources" DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
endif()

