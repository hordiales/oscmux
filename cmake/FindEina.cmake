# - Find libeina
# Find the libeina libraries 
#
#  This module defines the following variables:
#     EINA_FOUND       - True if EINA_INCLUDE_DIR & EINA_LIBRARY are found
#     EINA_LIBRARIES   - Set when EINA_LIBRARY is found
#     EINA_INCLUDE_DIRS - Set when EINA_INCLUDE_DIR is found
#
#     EINA_INCLUDE_DIR - where to find lo_lowlevel.h, etc.
#     EINA_LIBRARY     - the libeina library
#

find_package (PkgConfig)
pkg_search_module (PC_EINA eina QUIET)

find_path(EINA_INCLUDE_DIR NAMES Eina.h
    HINTS ${PC_EINA_INCLUDEDIR} ${PC_EINA_INCLUDE_DIRS}
    PATH_SUFFIXES eina-1
    DOC "The EINA include directory"
)
set (EINA_INCLUDE_DIR ${EINA_INCLUDE_DIR} "${EINA_INCLUDE_DIR}/eina")

find_library(EINA_LIBRARY NAMES eina
    HINTS ${PC_EINA_LIBDIR} ${PC_EINA_LIBRARY_DIRS}
    DOC "The EINA library"
)

# handle the QUIETLY and REQUIRED arguments and set EINA_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(EINA DEFAULT_MSG EINA_LIBRARY EINA_INCLUDE_DIR)

if(EINA_FOUND)
  set(EINA_LIBRARIES ${EINA_LIBRARY} )
  set(EINA_INCLUDE_DIRS ${EINA_INCLUDE_DIR} )
endif()

mark_as_advanced(EINA_INCLUDE_DIR EINA_LIBRARY)
