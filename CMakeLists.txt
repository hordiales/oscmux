cmake_minimum_required (VERSION 2.8)

project (oscmux)

set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set (OSCMUX_VERSION "0.1.0")

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
add_executable (oscmux oscmux.c)
target_link_libraries (oscmux ${LIBS})
install (TARGETS oscmux DESTINATION bin)
