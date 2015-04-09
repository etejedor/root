############################################################################
# CMakeLists.txt file for building ROOT tree/tree package
# @author Pere Mato, CERN
############################################################################

set(libname Tree)

ROOT_USE_PACKAGE(io/io)
ROOT_USE_PACKAGE(net/net)
ROOT_USE_PACKAGE(math/mathcore)
ROOT_USE_PACKAGE(core/metautils)
ROOT_USE_PACKAGE(core/thread)
ROOT_USE_PACKAGE(hist/hist)

ROOT_GENERATE_DICTIONARY(G__${libname} *.h MODULE ${libname} LINKDEF LinkDef.h OPTIONS "-writeEmptyRootPCM")
#ROOT_GENERATE_DICTIONARY(ManualTree TTree.h  LINKDEF LinkDef2.h)

INCLUDE_DIRECTORIES($ENV{INTEL_TBB}/include/)
FIND_LIBRARY(LIBTBB tbb HINTS $ENV{INTEL_TBB}/lib/intel64/gcc4.4/)
if(NOT LIBTBB)
  message(FATAL_ERROR "Library tbb not found")
endif()

ROOT_LINKER_LIBRARY(${libname} *.cxx G__${libname}.cxx DEPENDENCIES Net RIO Thread ${LIBTBB})
ROOT_INSTALL_HEADERS()

