# Find the interpreter as well for byte files generation
find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)

# Find pybind11
execute_process(
        COMMAND ${Python_EXECUTABLE} -c "import pybind11; print(pybind11.get_cmake_dir(), end='')"
        OUTPUT_VARIABLE pybind11_DIR
)
find_package(pybind11 CONFIG REQUIRED)

# Create C library
pybind11_add_module(bindings
        bindings.cpp
        ../pycdc.cpp
        ../ASTree.cpp
        ../ASTNode.cpp
)

target_include_directories(bindings PRIVATE pybind11::headers ${Python_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR})
target_link_libraries(bindings PRIVATE pycxx)

if (NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/lib")
endif ()

target_compile_definitions(
        bindings
        PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO})
