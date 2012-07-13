cmake_minimum_required (VERSION 2.8)

project (osc_redirect)

set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set (OSC_REDIRECT_VERSION "0.1.0")

# pkg-config
find_package (PkgConfig) # ${PKG_CONFIG_FOUND}

# eina
find_package (Eina)
include_directories (${EINA_INCLUDE_DIR})
set (LIBS ${LIBS} ${EINA_LIBRARY})

# liblo
pkg_search_module (LIBLO REQUIRED liblo)
include_directories (${LIBLO_INCLUDE_DIRS})
set (LIBS ${LIBS} ${LIBLO_LDFLAGS})

# executable
add_executable (osc_redirect osc_redirect.c)
target_link_libraries (osc_redirect ${LIBS})
install (TARGETS osc_redirect DESTINATION bin)
