#ifndef CPYCPPYY_CPYCPPYY_H
#define CPYCPPYY_CPYCPPYY_H

#ifdef _WIN32
// Disable warning C4275: non dll-interface class
#pragma warning (disable : 4275)
// Disable warning C4251: needs to have dll-interface to be used by clients
#pragma warning (disable : 4251)
// Disable warning C4800: 'int' : forcing value to bool
#pragma warning (disable : 4800)
// Avoid that pyconfig.h decides using a #pragma what library python library to use
//#define MS_NO_COREDLL 1
#endif

// to prevent problems with fpos_t and redefinition warnings
#if defined(linux)

#include <stdio.h>

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

#ifdef _FILE_OFFSET_BITS
#undef _FILE_OFFSET_BITS
#endif

#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif

#endif // linux


#include "Python.h"
#include <sys/types.h>

// selected ROOT types from RtypesCore.h
#ifdef R__INT16
typedef long           Int_t;       //Signed integer 4 bytes
typedef unsigned long  UInt_t;      //Unsigned integer 4 bytes
#else
typedef int            Int_t;       //Signed integer 4 bytes (int)
typedef unsigned int   UInt_t;      //Unsigned integer 4 bytes (unsigned int)
#endif
#ifdef R__B64    // Note: Long_t and ULong_t are currently not portable types
typedef long           Long_t;      //Signed long integer 8 bytes (long)
typedef unsigned long  ULong_t;     //Unsigned long integer 8 bytes (unsigned long)
#else
typedef long           Long_t;      //Signed long integer 4 bytes (long)
typedef unsigned long  ULong_t;     //Unsigned long integer 4 bytes (unsigned long)
#endif
typedef float          Float16_t;   //Float 4 bytes written with a truncated mantissa
typedef double         Double32_t;  //Double 8 bytes in memory, written as a 4 bytes float
typedef long double    LongDouble_t;//Long Double
#if defined(R__WIN32) && !defined(__CINT__)
typedef __int64          Long64_t;  //Portable signed long integer 8 bytes
typedef unsigned __int64 ULong64_t; //Portable unsigned long integer 8 bytes
#else
typedef long long          Long64_t; //Portable signed long integer 8 bytes
typedef unsigned long long ULong64_t;//Portable unsigned long integer 8 bytes
#endif


// for 3.3 support
#if PY_VERSION_HEX < 0x03030000
   typedef PyDictEntry* (*dict_lookup_func)(PyDictObject*, PyObject*, long);
#else
#if PY_VERSION_HEX >= 0x03060000
   typedef Py_ssize_t (*dict_lookup_func)(
       PyDictObject*, PyObject*, Py_hash_t, PyObject***, Py_ssize_t*);
#else
   struct PyDictKeyEntry;
   typedef PyDictKeyEntry* (*dict_lookup_func)(PyDictObject*, PyObject*, Py_hash_t, PyObject***);
#define PyDictEntry PyDictKeyEntry
#endif
#endif

// for 3.0 support (backwards compatibility, really)
#if PY_VERSION_HEX < 0x03000000
#define PyBytes_Check                  PyString_Check
#define PyBytes_CheckExact             PyString_CheckExact
#define PyBytes_AS_STRING              PyString_AS_STRING
#define PyBytes_AsString               PyString_AsString
#define PyBytes_GET_SIZE               PyString_GET_SIZE
#define PyBytes_Size                   PyString_Size
#define PyBytes_FromFormat             PyString_FromFormat
#define PyBytes_FromString             PyString_FromString
#define PyBytes_FromStringAndSize      PyString_FromStringAndSize

#define PyBytes_Type    PyString_Type

#define CPyCppyy_PyUnicode_Check              PyString_Check
#define CPyCppyy_PyUnicode_CheckExact         PyString_CheckExact
#define CPyCppyy_PyUnicode_AsString           PyString_AS_STRING
#define CPyCppyy_PyUnicode_AsStringChecked    PyString_AsString
#define CPyCppyy_PyUnicode_GET_SIZE           PyString_GET_SIZE
#define CPyCppyy_PyUnicode_GetSize            PyString_Size
#define CPyCppyy_PyUnicode_FromFormat         PyString_FromFormat
#define CPyCppyy_PyUnicode_FromString         PyString_FromString
#define CPyCppyy_PyUnicode_InternFromString   PyString_InternFromString
#define CPyCppyy_PyUnicode_Append             PyString_Concat
#define CPyCppyy_PyUnicode_AppendAndDel       PyString_ConcatAndDel
#define CPyCppyy_PyUnicode_FromStringAndSize  PyString_FromStringAndSize
static inline Py_ssize_t CPyCppyy_PyUnicode_AsWideChar(PyObject* pyobj, wchar_t* w, Py_ssize_t size)
{
     return PyUnicode_AsWideChar((PyUnicodeObject*)pyobj, w, size);
}

#define CPyCppyy_PyUnicode_Type PyString_Type

static inline PyObject* CPyCppyy_PyCapsule_New(
        void* cobj, const char* /* name */, void (*destr)(void*))
{
    return PyCObject_FromVoidPtr(cobj, destr);
}
#define CPyCppyy_PyCapsule_CheckExact    PyCObject_Check
static inline void* CPyCppyy_PyCapsule_GetPointer(PyObject* capsule, const char* /* name */)
{
    return (void*)PyCObject_AsVoidPtr(capsule);
}

#define CPPYY__long__ "__long__"
#define CPPYY__idiv__ "__idiv__"
#define CPPYY__div__  "__div__"
#define CPPYY__next__ "next"

#endif  // ! 3.0

// for 3.0 support (backwards compatibility, really)
#if PY_VERSION_HEX >= 0x03000000
#define CPyCppyy_PyUnicode_Check              PyUnicode_Check
#define CPyCppyy_PyUnicode_CheckExact         PyUnicode_CheckExact
#define CPyCppyy_PyUnicode_AsString           _PyUnicode_AsString
#define CPyCppyy_PyUnicode_AsStringChecked    _PyUnicode_AsString
#define CPyCppyy_PyUnicode_GetSize            PyUnicode_GetSize
#define CPyCppyy_PyUnicode_GET_SIZE           PyUnicode_GET_SIZE
#define CPyCppyy_PyUnicode_FromFormat         PyUnicode_FromFormat
#define CPyCppyy_PyUnicode_FromString         PyUnicode_FromString
#define CPyCppyy_PyUnicode_InternFromString   PyUnicode_InternFromString
#define CPyCppyy_PyUnicode_Append             PyUnicode_Append
#define CPyCppyy_PyUnicode_AppendAndDel       PyUnicode_AppendAndDel
#define CPyCppyy_PyUnicode_FromStringAndSize  PyUnicode_FromStringAndSize
#define CPyCppyy_PyUnicode_AsWideChar         PyUnicode_AsWideChar

#define CPyCppyy_PyUnicode_Type PyUnicode_Type

#define PyIntObject          PyLongObject
#define PyInt_Check          PyLong_Check
#define PyInt_AsLong         PyLong_AsLong
#define PyInt_AS_LONG        PyLong_AsLong
#define PyInt_AsSsize_t      PyLong_AsSsize_t
#define PyInt_CheckExact     PyLong_CheckExact
#define PyInt_FromLong       PyLong_FromLong
#define PyInt_FromSsize_t    PyLong_FromSsize_t

#define PyInt_Type      PyLong_Type

#define CPyCppyy_PyCapsule_New           PyCapsule_New
#define CPyCppyy_PyCapsule_CheckExact    PyCapsule_CheckExact
#define CPyCppyy_PyCapsule_GetPointer    PyCapsule_GetPointer

#define CPPYY__long__ "__int__"
#define CPPYY__idiv__ "__itruediv__"
#define CPPYY__div__  "__truediv__"
#define CPPYY__next__ "__next__"

#define Py_TPFLAGS_HAVE_RICHCOMPARE 0
#define Py_TPFLAGS_CHECKTYPES 0

#define PyClass_Check   PyType_Check

#define PyBuffer_Type   PyMemoryView_Type
#endif  // ! 3.0

#if PY_VERSION_HEX >= 0x03020000
#define CPyCppyy_PySliceCast   PyObject*
#define PyUnicode_GetSize      PyUnicode_GetLength
#else
#define CPyCppyy_PySliceCast   PySliceObject*
#endif  // >= 3.2

// feature of 3.0 not in 2.5 and earlier
#if PY_VERSION_HEX < 0x02060000
#define PyVarObject_HEAD_INIT(type, size)                                     \
    PyObject_HEAD_INIT(type) size,
#define Py_TYPE(ob)             (((PyObject*)(ob))->ob_type)
#endif

// backwards compatibility, pre python 2.5
#if PY_VERSION_HEX < 0x02050000
typedef int Py_ssize_t;
#define PyInt_AsSsize_t PyInt_AsLong
#define PyInt_FromSsize_t PyInt_FromLong
# define PY_SSIZE_T_FORMAT "%d"
# if !defined(PY_SSIZE_T_MIN)
#  define PY_SSIZE_T_MAX INT_MAX
#  define PY_SSIZE_T_MIN INT_MIN
# endif
#define ssizeobjargproc intobjargproc
#define lenfunc         inquiry
#define ssizeargfunc    intargfunc

#define PyIndex_Check(obj)                                                    \
    (PyInt_Check(obj) || PyLong_Check(obj))

inline Py_ssize_t PyNumber_AsSsize_t(PyObject* obj, PyObject*) {
    return (Py_ssize_t)PyLong_AsLong(obj);
}

#else
# ifdef R__MACOSX
#  if SIZEOF_SIZE_T == SIZEOF_INT
#    if defined(MAC_OS_X_VERSION_10_4)
#       define PY_SSIZE_T_FORMAT "%ld"
#    else
#       define PY_SSIZE_T_FORMAT "%d"
#    endif
#  elif SIZEOF_SIZE_T == SIZEOF_LONG
#    define PY_SSIZE_T_FORMAT "%ld"
#  endif
# else
#  define PY_SSIZE_T_FORMAT "%zd"
# endif
#endif

#if PY_VERSION_HEX < 0x02020000
#define PyBool_FromLong  PyInt_FromLong
#endif

#if PY_VERSION_HEX < 0x03000000
// the following should quiet Solaris
#ifdef Py_False
#undef Py_False
#define Py_False ((PyObject*)(void*)&_Py_ZeroStruct)
#endif

#ifdef Py_True
#undef Py_True
#define Py_True ((PyObject*)(void*)&_Py_TrueStruct)
#endif
#endif

#ifndef Py_RETURN_NONE
#define Py_RETURN_NONE return Py_INCREF(Py_None), Py_None
#endif

#ifndef Py_RETURN_TRUE
#define Py_RETURN_TRUE return Py_INCREF(Py_True), Py_True
#endif

#ifndef Py_RETURN_FALSE
#define Py_RETURN_FALSE return Py_INCREF(Py_False), Py_False
#endif

// C++ version of the cppyy API
#include "Cppyy.h"

#endif // !CPYCPPYY_CPYCPPYY_H
