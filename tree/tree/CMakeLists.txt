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

INCLUDE_DIRECTORIES($ENV{EXTRAE_HOME}/include/)
FIND_LIBRARY(LIBEXTRAE nanostrace HINTS $ENV{EXTRAE_HOME}/lib/)
if(NOT LIBEXTRAE)
  message(FATAL_ERROR "Library nanostrace not found")
endif()

INCLUDE_DIRECTORIES(${TBB_INCLUDE_DIRS})

ROOT_LINKER_LIBRARY(${libname} *.cxx G__${libname}.cxx LIBRARIES ${TBB_LIBRARY_RELEASE} ${LIBEXTRAE} DEPENDENCIES Net RIO Thread)
ROOT_INSTALL_HEADERS()

