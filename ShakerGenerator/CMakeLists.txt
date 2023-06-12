cmake_minimum_required(VERSION 3.0)

project (ShakerGenerator)

include_directories(PUBLIC ${CPCCORE} ${PROJECT_SOURCE_DIR}/../SFML/include)

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

add_executable( ShakerGenerator  
                Display.cpp 
                Display.h 
                TestUtils.h
                TestUtils.cpp
                Test_Shaker_generation.cpp 
)

target_link_libraries(ShakerGenerator PUBLIC zlibstatic CPCCoreEmu sfml-graphics ${LIBZA} )

install(TARGETS ShakerGenerator 
            RUNTIME DESTINATION "."
            LIBRARY DESTINATION "."
            ARCHIVE DESTINATION lib/static)

