#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"

#include <Python.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <vector>
#include <cstring>
#include <ostream>
#include <sstream>
#include <optional>
#include "ASTree.h"

namespace py = pybind11;


#ifdef WIN32
#  define PATHSEP '\\'
#else
#  define PATHSEP '/'
#endif

py::str decompyle_binding(py::bytes &data, int major_version, int minor_version) {
    PycModule mod;
    auto str = data.cast<std::string>();
    PycBuffer buffer(
            reinterpret_cast<const unsigned char*>(str.c_str()),
            str.size()
    );

    if (major_version == 0 && minor_version == 0) {
        mod.loadFromStream(buffer);
    }
    else {
        mod.loadFromMarshalledStream(
                buffer,
                major_version,
                minor_version
        );
    }
    std::ostringstream pyc_output;
    decompyle(mod.code(), &mod, pyc_output);
    return pyc_output.str();
}

PYBIND11_MODULE(bindings, m) {
    m.doc() = "pycdcpy bindings";
    m.def("decompyle", &decompyle_binding, "Decompile a marshalled python file");
}
