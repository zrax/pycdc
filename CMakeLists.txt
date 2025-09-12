cmake_minimum_required(VERSION 3.12)
project(pycdc)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Debug options
option(ENABLE_BLOCK_DEBUG "Enable block debugging" OFF)
option(ENABLE_STACK_DEBUG "Enable stack debugging" OFF)

# Turn debug defs on if they're enabled.
if (ENABLE_BLOCK_DEBUG)
    add_definitions(-DBLOCK_DEBUG)
endif()
if (ENABLE_STACK_DEBUG)
    add_definitions(-DSTACK_DEBUG)
endif()

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Werror -Wno-error=shadow)
elseif(MSVC)
    add_compile_options(/WX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

file(GLOB SRC_LIB_CPP CONFIGURE_DEPENDS
    bytecode.cpp
    data.cpp
    pyc_code.cpp
    pyc_module.cpp
    pyc_numeric.cpp
    pyc_object.cpp
    pyc_sequence.cpp
    pyc_string.cpp
    bytes/*.cpp
)

file(GLOB SRC_MAIN CONFIGURE_DEPENDS
    *.h
    bytes/*.h
    pycdas.cpp
    pycdc.cpp
    ASTree.cpp
    ASTNode.cpp
    ${SRC_LIB_CPP})

file(GLOB SRC_ALL CONFIGURE_DEPENDS
    bytecode_ops.inl # This file does not contain compilable code
    ${SRC_MAIN}
)

# Enable/disable targets/tools
option(ENABLE_CLANG_FORMAT "Provide `format` and `format-check` targets using clang-format" ON)
option(ENABLE_CLANG_TIDY  "Provide `tidy-all` target using clang-tidy" ON)

# Export compile commands for clang-tidy / language servers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(ENABLE_CLANG_FORMAT)
    find_program(CLANG_FORMAT_EXECUTABLE
        NAMES clang-format
    )
    if(CLANG_FORMAT_EXECUTABLE)
        add_custom_target(format
            COMMAND ${CLANG_FORMAT_EXECUTABLE} -style=file -i ${SRC_ALL}
            COMMENT "Running clang-format on project sources"
            VERBATIM
        )

        add_custom_target(format-check
            COMMAND ${CLANG_FORMAT_EXECUTABLE} -style=file --dry-run -Werror ${SRC_ALL}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Checking clang-format (non-zero exit if formatting required)"
            VERBATIM
            USES_TERMINAL # Enables propagating error code
        )
    endif()
endif()

if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXECUTABLE
        NAMES clang-tidy
    )
    if(CLANG_TIDY_EXECUTABLE)
        # Minimal clang-tidy wrapper. Adjust checks in your .clang-tidy file.
        # Semicolon-separated list is required by CMake for the wrapper.
        set(CMAKE_CXX_CLANG_TIDY
            "${CLANG_TIDY_EXECUTABLE};-p=${CMAKE_BINARY_DIR}"
        )
        # If you prefer to apply clang-tidy to specific targets only, remove the
        # global set above and use set_target_properties(... CXX_CLANG_TIDY ...)
        # Create a custom target to run clang-tidy over main sources
        add_custom_target(tidy-all
            COMMAND ${CLANG_TIDY_EXECUTABLE} -p ${CMAKE_BINARY_DIR} ${SRC_MAIN}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running clang-tidy on project sources"
            VERBATIM
        )
    endif()
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Library
add_library(pycxx STATIC
    ${SRC_LIB_CPP}
)

# Executable: pycdas
add_executable(pycdas pycdas.cpp)
target_link_libraries(pycdas PRIVATE pycxx)

install(TARGETS pycdas RUNTIME DESTINATION bin)

# Executable: pycdc
add_executable(pycdc pycdc.cpp ASTree.cpp ASTNode.cpp)
target_link_libraries(pycdc PRIVATE pycxx)

install(TARGETS pycdc RUNTIME DESTINATION bin)

find_package(Python3 3.6 COMPONENTS Interpreter)
if(Python3_FOUND)
    add_custom_target(check
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.py"
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:pycdc>")
    add_dependencies(check pycdc)
endif()
