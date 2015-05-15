############################################################################
# CMakeLists.txt file for building ROOT core/instrument package
############################################################################

INCLUDE_DIRECTORIES($ENV{EXTRAE_HOME}/include/)

FIND_LIBRARY(LIBEXTRAE nanostrace HINTS $ENV{EXTRAE_HOME}/lib/)
if(NOT LIBEXTRAE)
  message(FATAL_ERROR "Library nanostrace not found")
endif()

ROOT_LINKER_LIBRARY(ExtraeInstrumenter *.cxx LIBRARIES ${LIBEXTRAE})
ROOT_INSTALL_HEADERS()
