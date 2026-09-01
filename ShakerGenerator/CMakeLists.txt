cmake_minimum_required(VERSION 3.16)

project(ShakerGenerator)

add_executable(ShakerGenerator
    Display.cpp
    Display.h
    TestUtils.h
    TestUtils.cpp
    Test_Shaker_generation.cpp
)

target_compile_features(ShakerGenerator PRIVATE cxx_std_17)

target_include_directories(ShakerGenerator PRIVATE
    ${CPCCORE}
    ${PROJECT_SOURCE_DIR}/../SFML/include
)

if(MSVC)
    target_compile_definitions(ShakerGenerator PRIVATE SAFESEH=NO)
    target_link_options(ShakerGenerator PRIVATE /SAFESEH:NO)
endif()

target_link_libraries(ShakerGenerator PUBLIC zlibstatic CPCCoreEmu sfml-graphics ${LIBZA})

install(TARGETS ShakerGenerator
    RUNTIME DESTINATION "./Shaker"
    LIBRARY DESTINATION "./Shaker"
    ARCHIVE DESTINATION lib/static)

install(FILES "./TestConf.ini"   DESTINATION "./Shaker")
install(FILES "./TestConf_0.ini" DESTINATION "./Shaker")
install(FILES "./TestConf_1.ini" DESTINATION "./Shaker")
install(FILES "./TestConf_2.ini" DESTINATION "./Shaker")
install(FILES "./TestConf_3.ini" DESTINATION "./Shaker")
install(FILES "./TestConf_4.ini" DESTINATION "./Shaker")

install(DIRECTORY "../UnitTests/ROM"        DESTINATION "./Shaker")
install(DIRECTORY "../UnitTests/CART"       DESTINATION "./Shaker")
install(DIRECTORY "../UnitTests/CONF"       DESTINATION "./Shaker")
install(DIRECTORY "../UnitTests/res/Shaker" DESTINATION "./Shaker")
