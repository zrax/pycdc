cmake_minimum_required(VERSION 3.12)
project(pycdc)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Debug options.
option(ENABLE_BLOCK_DEBUG "Enable block debugging" OFF)
option(ENABLE_STACK_DEBUG "Enable stack debugging" OFF)

# Turn debug defs on if they're enabled.
if (ENABLE_BLOCK_DEBUG)
    add_definitions(-DBLOCK_DEBUG)
endif()
if (ENABLE_STACK_DEBUG)
    add_definitions(-DSTACK_DEBUG)
endif()

# For generating the bytes tables
find_package(Python COMPONENTS Interpreter REQUIRED)

if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wshadow -Werror ${CMAKE_CXX_FLAGS}")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "/WX ${CMAKE_CXX_FLAGS}")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(PYTHON_VERSIONS
    10 11    13 14 15 16            # Python 1.1 and 1.2 are marshal-identical
    20 21 22 23 24 25 26 27
    30 31 32 33 34 35 36 37 38 39 310 311 312
)

foreach(ver ${PYTHON_VERSIONS})
    list(APPEND MAP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/bytes/python_${ver}.map)
    list(APPEND MAP_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/bytes/python_${ver}.cpp)
endforeach()

add_custom_command(OUTPUT ${MAP_SOURCES}
                   COMMAND ${Python_EXECUTABLE}
                           ${CMAKE_CURRENT_SOURCE_DIR}/bytes/comp_map.py
                           ${CMAKE_CURRENT_SOURCE_DIR}/bytes
                           ${CMAKE_CURRENT_BINARY_DIR}/bytes
                   DEPENDS ${MAP_FILES}
                           ${CMAKE_CURRENT_SOURCE_DIR}/bytes/comp_map.py
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(pycxx STATIC
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

add_executable(pycdas pycdas.cpp)
target_link_libraries(pycdas pycxx)

install(TARGETS pycdas
    RUNTIME DESTINATION bin)

add_executable(pycdc pycdc.cpp ASTree.cpp ASTNode.cpp)
target_link_libraries(pycdc pycxx)

install(TARGETS pycdc
    RUNTIME DESTINATION bin)

add_custom_target(check "${CMAKE_CURRENT_SOURCE_DIR}/tests/all_tests.sh"
                  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
add_dependencies(check pycdc)
