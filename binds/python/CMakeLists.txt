if (NOT EXTERNAL_PYBIND11)
    add_subdirectory(pybind11 EXCLUDE_FROM_ALL)
endif()

pybind11_add_module(_morphio SYSTEM
    morphio.cpp
    bind_immutable.cpp
    bindings_utils.cpp
    bind_misc.cpp
    bind_mutable.cpp
    bind_vasculature.cpp
)

target_link_libraries(_morphio
    PRIVATE morphio_static
    PRIVATE pybind11::module
)
