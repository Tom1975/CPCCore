cmake_minimum_required(VERSION 3.16...3.28)

project(AmstradCoreTests)

include(GoogleTest)
enable_testing()

add_executable(AmstradCoreTests
    Test_GateArray.cpp
    Test_Z80.cpp
)

target_compile_features(AmstradCoreTests PRIVATE cxx_std_17)

target_include_directories(AmstradCoreTests PRIVATE
    ${gtest_SOURCE_DIR}/include
    ${gtest_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/../AmstradCore
)

if(MSVC)
    target_compile_definitions(AmstradCoreTests PRIVATE SAFESEH=NO)
    target_link_options(AmstradCoreTests PRIVATE /SAFESEH:NO)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(LIBZA libz.a)
elseif(CMAKE_COMPILER_IS_GNUCXX)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
        set(LIBZA libz.a stdc++fs)
    else()
        set(LIBZA libz.a)
    endif()
endif()

target_link_libraries(AmstradCoreTests PUBLIC zlibstatic gtest gtest_main AmstradCore ${LIBZA})

gtest_discover_tests(AmstradCoreTests
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/.."
)

install(TARGETS AmstradCoreTests
    RUNTIME DESTINATION "."
    LIBRARY DESTINATION "."
    ARCHIVE DESTINATION lib/static)
