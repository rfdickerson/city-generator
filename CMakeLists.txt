cmake_minimum_required(VERSION 4.1)
project(buildings)

set(CMAKE_CXX_STANDARD 20)

add_executable(buildings
    main.cpp
    math.h
    geometry.h
    geometry.cpp
    geom_ops.h
    geom_ops.cpp
    mesh.h
    mesh.cpp
    semantics.h
    semantics.cpp
    style_midcentury.h
    style_midcentury.cpp
    style_brutalist.h
    style_brutalist.cpp
    io.h
    io.cpp
    json.h
    json.cpp
    config.h
    config.cpp
)

add_executable(city_builder
    city_main.cpp
    city_builder.h
    city_builder.cpp
    city_config.h
    city_config.cpp
    math.h
    geometry.h
    geometry.cpp
    geom_ops.h
    geom_ops.cpp
    mesh.h
    mesh.cpp
    semantics.h
    semantics.cpp
    style_midcentury.h
    style_midcentury.cpp
    style_brutalist.h
    style_brutalist.cpp
    io.h
    io.cpp
    json.h
    json.cpp
    config.h
    config.cpp
)

enable_testing()

add_test(
    NAME generate_obj
    COMMAND
        ${CMAKE_COMMAND}
        -DAPP=$<TARGET_FILE:buildings>
        -DOUT=simcity_midcentury_office.obj
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateAndVerifyObj.cmake
)
set_tests_properties(generate_obj PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
