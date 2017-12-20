project(pycdc)
cmake_minimum_required(VERSION 2.8)

# For generating the bytes tables
find_package(PythonInterp REQUIRED)

if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror ${CMAKE_CXX_FLAGS}")
endif()

set(PYTHON_VERSIONS
    10 11    13 14 15 16        # Python 1.1 and 1.2 are marshal-identical
    20 21 22 23 24 25 26 27
    30 31 32 33 34 35 36 37
)

foreach(ver ${PYTHON_VERSIONS})
    set(MAP_FILES   ${MAP_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/bytes/python_${ver}.map)
    set(MAP_SOURCES ${MAP_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/bytes/python_${ver}.cpp)
endforeach()

add_custom_command(OUTPUT ${MAP_SOURCES}
                   COMMAND ${PYTHON_EXECUTABLE}
                           ${CMAKE_CURRENT_SOURCE_DIR}/bytes/comp_map.py
                           ${CMAKE_CURRENT_SOURCE_DIR}/bytes
                           ${CMAKE_CURRENT_BINARY_DIR}/bytes
                   DEPENDS ${MAP_FILES}
                           ${CMAKE_CURRENT_SOURCE_DIR}/bytes/comp_map.py
)

set(COMMON_SOURCES
    bytecode.cpp
    data.cpp
    pyc_code.cpp
    pyc_module.cpp
    pyc_numeric.cpp
    pyc_object.cpp
    pyc_sequence.cpp
    pyc_string.cpp
    ${MAP_SOURCES}
)

set(pycdc_AST_SOURCES
    ASTree.cpp
    ASTNode.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_library(pycxx STATIC ${COMMON_SOURCES})

add_executable(pycdas pycdas.cpp)
target_link_libraries(pycdas pycxx)

install(TARGETS pycdas
    RUNTIME DESTINATION bin)

add_executable(pycdc pycdc.cpp ${pycdc_AST_SOURCES})
target_link_libraries(pycdc pycxx)

install(TARGETS pycdc
    RUNTIME DESTINATION bin)

# For tests
if(POLICY CMP0037)
    # Don't complain about adding a target named "test"
    cmake_policy(SET CMP0037 OLD)
endif()

add_custom_target(test "${CMAKE_CURRENT_SOURCE_DIR}/pycdc_test.sh"
                       "${CMAKE_CURRENT_SOURCE_DIR}/tests"
                  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
add_dependencies(test pycdc)

add_custom_target(rt_test "${CMAKE_CURRENT_SOURCE_DIR}/pycdc_rt_test.sh"
                          "${CMAKE_CURRENT_BINARY_DIR}/tests"
                  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
add_dependencies(rt_test test)
