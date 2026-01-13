cmake_minimum_required(VERSION 3.25)
project(buildings)

set(CMAKE_CXX_STANDARD 20)

add_executable(buildings
    src/main.cpp
    src/math.h
    src/geometry.h
    src/geometry.cpp
    src/geom_ops.h
    src/geom_ops.cpp
    src/mesh.h
    src/mesh.cpp
    src/semantics.h
    src/semantics.cpp
    src/style_midcentury.h
    src/style_midcentury.cpp
    src/style_brutalist.h
    src/style_brutalist.cpp
    src/io.h
    src/io.cpp
    src/json.h
    src/json.cpp
    src/config.h
    src/config.cpp
    src/instances.h
)

add_executable(city_builder
    src/city_main.cpp
    src/city_builder.h
    src/city_builder.cpp
    src/city_config.h
    src/city_config.cpp
    src/math.h
    src/geometry.h
    src/geometry.cpp
    src/geom_ops.h
    src/geom_ops.cpp
    src/mesh.h
    src/mesh.cpp
    src/semantics.h
    src/semantics.cpp
    src/style_midcentury.h
    src/style_midcentury.cpp
    src/style_brutalist.h
    src/style_brutalist.cpp
    src/io.h
    src/io.cpp
    src/json.h
    src/json.cpp
    src/config.h
    src/config.cpp
    src/instances.h
)


enable_testing()

include(FetchContent)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(test_geometry
    tests/test_geometry.cpp
    src/geometry.cpp
)
target_include_directories(test_geometry PRIVATE src)
target_link_libraries(test_geometry PRIVATE GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(test_geometry)

add_test(
    NAME generate_obj
    COMMAND
        ${CMAKE_COMMAND}
        -DAPP=$<TARGET_FILE:buildings>
        -DOUT=simcity_midcentury_office.obj
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/GenerateAndVerifyObj.cmake
)
set_tests_properties(generate_obj PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
