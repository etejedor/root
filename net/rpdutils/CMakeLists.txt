############################################################################
# CMakeLists.txt file for building ROOT net/rpdutils package
# @author Pere Mato, CERN
############################################################################

ROOT_GLOB_SOURCES(rpdutilsrcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx)
list(REMOVE_ITEM rpdutilsrcs src/rpdpriv.cxx src/rpdconn.cxx)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/res
  ${CMAKE_CURRENT_SOURCE_DIR}/../auth/res)

if(MACOSX_SSL_DEPRECATED)
  set_source_files_properties(src/rpdutils.cxx COMPILE_FLAGS "-Wno-deprecated-declarations" )
endif()

#---Authentication methods-----------------------------------------------------------------
if(shadowpw)
  add_definitions(-DR__SHADOWPW)
endif()

if(ssl)
  include_directories(${OPENSSL_INCLUDE_DIR})
endif()

#---static library needed for rootd and proofd --------------------------------------------
add_library(rpdutil STATIC ${rpdutilsrcs})
target_link_libraries(rpdutil INTERFACE ${CRYPTLIBS})
set_property(TARGET rpdutil PROPERTY POSITION_INDEPENDENT_CODE ON)
add_dependencies(rpdutil move_headers)

ROOT_LINKER_LIBRARY(SrvAuth rpdutils.cxx
  LIBRARIES rpdutil rsa ${OPENSSL_LIBRARIES}
  DEPENDENCIES Net
  BUILTINS OPENSSL)
