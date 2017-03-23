# -*- coding: utf-8 -*-
# =====================================================================================================================
# These bindings were automatically generated by cyWrap. Please do dot modify.
# Additional functionality shall be implemented in sub-classes.
#
from __future__ import print_function
__copyright__ = "Copyright 2016 EPFL BBP-project"
# =====================================================================================================================
from cython.operator cimport dereference as deref
cimport std
from libcpp cimport bool
from libc.string cimport memcpy

cimport morpho
cimport morpho_h5_v1
from statics cimport morpho_morpho_mesher
from statics cimport morpho_h5_v1_morpho_reader

import numpy as np
cimport numpy as np


# We need to initialize NumPy.
np.import_array()


# --------------------- BASE CLASS -----------------------
cdef enum OPERATOR:
    LESS = 0, LESS_EQUAL, EQUAL, DIFF, GREATER, GREATER_EQUAL

cdef class _py__base:
    cdef void *_ptr
    # Basic comparison is done by comparing the inner obj ptr
    def __richcmp__(_py__base self, _py__base other, operation):
        if operation == OPERATOR.EQUAL:
            return self._ptr==other._ptr

cdef class _Enum:
    def __cinit__(self):
        raise TypeError("Cant instantiate Enum")

    @classmethod
    def get_description(cls, int item):
        for name, value in cls.__dict__.items():
            if not name.startswith("_") and value==item:
                return name
        raise IndexError("No such Enumerator index")

#//-- BASE CLASS ------------------------------------------

## Include data structures
include "datastructs.pxi"


# ======================================================================================================================
# Python bindings to namespace morpho
# ======================================================================================================================


# ======================================================================================================================
cdef class BRANCH_TYPE(_Enum):
    soma = morpho.soma
    axon = morpho.axon
    dentrite_basal = morpho.dentrite_basal
    dentrite_apical = morpho.dentrite_apical
    unknown = morpho.unknown

# ----------------------------------------------------------------------------------------------------------------------
cdef class Morpho_Node(_py__base):
    "Python wrapper class for morpho_node (ns=morpho)"
# ----------------------------------------------------------------------------------------------------------------------
    cdef morpho.morpho_node *ptr_(self):
        return <morpho.morpho_node*> self._ptr

    def __init__(self, int my_node_type):
        self._ptr = new morpho.morpho_node(<morpho.branch_type> my_node_type)

    def get_type(self, ):
        return self.ptr_().get_type()

    @staticmethod
    cdef Morpho_Node from_ptr(morpho.morpho_node *ptr):
        cdef Morpho_Node obj = Morpho_Node.__new__(Morpho_Node)
        obj._ptr = ptr
        return obj

    @staticmethod
    cdef Morpho_Node from_ref(const morpho.morpho_node &ref):
        return Morpho_Node.from_ptr(<morpho.morpho_node*>&ref)

    @staticmethod
    cdef list vector2list( std.vector[morpho.morpho_node*] vec ):
        return [ Morpho_Node.from_ptr(elem) for elem in vec ]



cdef class Branch(Morpho_Node):
    "Python wrapper class for branch (ns=morpho)"
# ----------------------------------------------------------------------------------------------------------------------
    cdef morpho.branch *ptr(self):
        return <morpho.branch*> self._ptr

    # def set_points(self, _py_mat_points points, _py_vec_double distances):
    def set_points(self, ):
        raise NotImplementedError("Set points is not available in the Python API")

    def get_points(self, ):
        return Mat_Points.from_ref( self.ptr().get_points() )

    def get_size(self, ):
        return self.ptr().get_size()

    def get_point(self, std.size_t id_):
        # Point object is volatile,
        cdef morpho.point p = self.ptr().get_point(id_)
        # so we must create a python object which reserves memory
        cdef np.ndarray[np.double_t] arr = np.empty(3, "d")
        memcpy( arr.data, p.data(), 3*sizeof(double) )
        return arr

    def get_bounding_box(self, ):
        return Box.from_value(self.ptr().get_bounding_box())

    def get_segment(self, std.size_t n):
        return Cone.from_value(self.ptr().get_segment(n))

    def get_segment_bounding_box(self, std.size_t n):
        return Box.from_value(self.ptr().get_segment_bounding_box(n))

    def get_junction(self, std.size_t n):
        return Sphere.from_value(self.ptr().get_junction(n))

    def get_junction_sphere_bounding_box(self, std.size_t n):
        return Box.from_value(self.ptr().get_junction_sphere_bounding_box(n))

    def get_linestring(self, ):
        return Linestring.from_value(self.ptr().get_linestring())

    def get_circle_pipe(self, ):
        return CirclePipe.from_value( self.ptr().get_circle_pipe() )

    def get_childrens(self, ):
        return self.ptr().get_childrens()

    @property
    def parent(self, ):
        return self.ptr().get_parent()

    @property
    def id(self, ):
        return self.ptr().get_id()

    @staticmethod
    cdef Branch from_ptr(morpho.branch *ptr):
        cdef Branch obj = Branch.__new__(Branch)
        obj._ptr = ptr
        return obj
    
    @staticmethod
    cdef Branch from_ref(const morpho.branch &ref):
        return Branch.from_ptr(<morpho.branch*>&ref)

    @staticmethod
    cdef list vector2list( std.vector[morpho.branch*] vec ):
        return [ Branch.from_ptr(elem) for elem in vec ]



# ----------------------------------------------------------------------------------------------------------------------
cdef class Branch_Soma(Branch):
    "Python wrapper class for branch_soma (ns=morpho)"
# ----------------------------------------------------------------------------------------------------------------------
    cdef morpho.branch_soma *ptrx(self):
        return <morpho.branch_soma*> self._ptr

    #def __init__(self, ):
    #    self._ptr = new morpho.branch_soma()
    #    self._autodealoc.reset(self.ptrx())

    def get_sphere(self, ):
        return Sphere.from_value(self.ptrx().get_sphere())

    def get_bounding_box(self, ):
        return Box.from_value(self.ptrx().get_bounding_box())

    @staticmethod
    cdef Branch_Soma from_ptr(morpho.branch_soma *ptr):
        cdef Branch_Soma obj = Branch_Soma.__new__(Branch_Soma)
        obj._ptr = ptr
        return obj
    
    @staticmethod
    cdef Branch_Soma from_ref(const morpho.branch_soma &ref):
        return Branch_Soma.from_ptr(<morpho.branch_soma*>&ref)

    @staticmethod
    cdef list vector2list( std.vector[morpho.branch_soma*] vec ):
        return [ Branch_Soma.from_ptr(elem) for elem in vec ]



# ----------------------------------------------------------------------------------------------------------------------
cdef class Morpho_Tree(_py__base):
    "Python wrapper class for morpho_tree (ns=morpho)"
# ----------------------------------------------------------------------------------------------------------------------
    cdef std.unique_ptr[morpho.morpho_tree] _autodealoc
    cdef morpho.morpho_tree *ptr(self):
        return <morpho.morpho_tree*> self._ptr


    def __init__(self, ):
        self._ptr = new morpho.morpho_tree()
        #self._autodealoc.reset(self.ptr())


    #def __init__(self, _py_morpho_tree other):
    #    self._ptr = new morpho.morpho_tree(deref(other.ptr()))
    #    self._autodealoc.reset(self.ptr())

    #Error
    # def set_root(self, Branch root_elem):
    #     cdef std.unique_ptr[morpho.branch] root_ptr
    #     root_ptr.reset(root_elem.ptr())
    #     return self.ptr().set_root(std.move(root_ptr))

    #Error
    # def add_child(self, std.size_t parent_id, Branch children):
    #     cdef std.unique_ptr[morpho.branch] child_ptr
    #     child_ptr.reset(children.ptr())
    #     return self.ptr().add_child(parent_id, std.move(child_ptr))


    def get_branch(self, std.size_t id_branch):
        return Branch.from_ref(self.ptr().get_branch(id_branch))

    def get_bounding_box(self, ):
        return Box.from_value(self.ptr().get_bounding_box())

    def get_tree_size(self, ):
        return self.ptr().get_tree_size()

    def add_flag(self, int flag):
        return self.ptr().add_flag(flag)

    def get_flags(self, ):
        return self.ptr().get_flags()

    @staticmethod
    cdef Morpho_Tree from_ptr(morpho.morpho_tree *ptr):
        cdef Morpho_Tree obj = Morpho_Tree.__new__(Morpho_Tree)
        obj._ptr = ptr
        return obj
    
    @staticmethod
    cdef Morpho_Tree from_ref(const morpho.morpho_tree &ref):
        return Morpho_Tree.from_ptr(<morpho.morpho_tree*>&ref)

    @staticmethod
    cdef Morpho_Tree from_move(morpho.morpho_tree &ref):
        cdef Morpho_Tree obj = Morpho_Tree()
        obj.ptr().swap(ref)
        return obj


    @staticmethod
    cdef list vector2list( std.vector[morpho.morpho_tree*] vec ):
        return [ Morpho_Tree.from_ptr(elem) for elem in vec ]


# ======================================================================================================================
# Python bindings to namespace morpho::h5_v1
# ======================================================================================================================

cdef class Morpho_Reader_Mat_Index(_py__base):
    cdef morpho_h5_v1_morpho_reader.mat_index* ptr(self):
        return <morpho_h5_v1_morpho_reader.mat_index *> self._ptr

    @staticmethod
    cdef Morpho_Reader_Mat_Index from_ptr(morpho_h5_v1_morpho_reader.mat_index *ptr):
        cdef Morpho_Reader_Mat_Index obj = Morpho_Reader_Mat_Index.__new__(Morpho_Reader_Mat_Index)
        obj._ptr = ptr
        return obj

    @staticmethod
    cdef Morpho_Reader_Mat_Index from_ref(const morpho_h5_v1_morpho_reader.mat_index &ref):
        return Morpho_Reader_Mat_Index.from_ptr(<morpho_h5_v1_morpho_reader.mat_index*>&ref)


# ----------------------------------------------------------------------------------------------------------------------
cdef class Morpho_Reader(_py__base):
    "Python wrapper class for morpho_reader (ns=morpho::h5_v1)"
# ----------------------------------------------------------------------------------------------------------------------
    pass
    cdef std.unique_ptr[morpho_h5_v1.morpho_reader] _autodealoc
    cdef morpho_h5_v1.morpho_reader *ptr(self):
        return <morpho_h5_v1.morpho_reader*> self._ptr

    def __init__(self, std.string filename):
        self._ptr = new morpho_h5_v1.morpho_reader(filename)
        self._autodealoc.reset(self.ptr())

    def get_points_raw(self, ):
        return Mat_Points.from_ref(self.ptr().get_points_raw())

    def get_soma_points_raw(self, ):
        return Mat_Points.from_ref(self.ptr().get_soma_points_raw())

    def get_struct_raw(self, ):
        return Morpho_Reader_Mat_Index.from_ref(self.ptr().get_struct_raw())

    def get_branch_range_raw(self, int id_):
        return self.ptr().get_branch_range_raw(id_)

    def get_filename(self, ):
        return self.ptr().get_filename()

    def create_morpho_tree(self, ):
        cdef morpho.morpho_tree tree = self.ptr().create_morpho_tree()
        return Morpho_Tree.from_move(tree)

    @staticmethod
    cdef Morpho_Reader from_ptr(morpho_h5_v1.morpho_reader *ptr):
        cdef Morpho_Reader obj = Morpho_Reader.__new__(Morpho_Reader)
        obj._ptr = ptr
        return obj

    @staticmethod
    cdef Morpho_Reader from_ref(const morpho_h5_v1.morpho_reader &ref):
        return Morpho_Reader.from_ptr(<morpho_h5_v1.morpho_reader*>&ref)

    @staticmethod
    cdef list vector2list( std.vector[morpho_h5_v1.morpho_reader*] vec ):
        return [ Morpho_Reader.from_ptr(elem) for elem in vec ]

