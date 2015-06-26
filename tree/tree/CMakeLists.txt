############################################################################
# CMakeLists.txt file for building ROOT tree/tree package
# @author Pere Mato, CERN
############################################################################

set(libname Tree)

ROOT_USE_PACKAGE(io/io)
ROOT_USE_PACKAGE(net/net)
ROOT_USE_PACKAGE(math/mathcore)
ROOT_USE_PACKAGE(core/instrument)
ROOT_USE_PACKAGE(core/metautils)
ROOT_USE_PACKAGE(core/thread)
ROOT_USE_PACKAGE(hist/hist)

ROOT_GENERATE_DICTIONARY(G__${libname} *.h MODULE ${libname} LINKDEF LinkDef.h OPTIONS "-writeEmptyRootPCM")
#ROOT_GENERATE_DICTIONARY(ManualTree TTree.h  LINKDEF LinkDef2.h)

INCLUDE_DIRECTORIES(${TBB_INCLUDE_DIRS})

# TODO: REMOVE HIST and GPAD DEPS!!!!!
ROOT_LINKER_LIBRARY(${libname} *.cxx G__${libname}.cxx LIBRARIES ${TBB_LIBRARY_RELEASE} DEPENDENCIES ExtraeInstrumenter Net RIO Thread Hist Gpad)
ROOT_INSTALL_HEADERS()

