cmake_minimum_required(VERSION 4.1)
project(buildings)

set(CMAKE_CXX_STANDARD 20)

add_executable(buildings
    main.cpp
    math.h
    geometry.h
    geometry.cpp
    mesh.h
    mesh.cpp
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
