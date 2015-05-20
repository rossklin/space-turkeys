#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([Space Turkeys], [3.0], [rossklin@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([solar.h])
# AC_CONFIG_HEADERS([config.h])
# AC_SUBST(print_date, "echo $(shell date)")

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
# AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_CHECK_TOOL([CMAKE], [cmake])
# check for doxygen
AC_CHECK_PROGS([DOXYGEN], [doxygen])
if test -z "$DOXYGEN";
   then AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])
fi

# configure external sfml with cmake
export wd=$(pwd)

# sfml
cd $wd/external/src/SFML
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr/local .

# tgui
cp $wd/tgui_makelists.txt $wd/external/src/TGUI/CMakeLists.txt
cd $wd/external/src/TGUI
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr/local -DSFML_INCLUDE_DIR=../SFML/include .

cd $wd				 

# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([xcb-image], [xcb_image_annotate])

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h limits.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([clock_gettime floor inet_ntoa memmove memset pow select socket sqrt strchr strstr strtol strtoul])

# set include paths
CXXFLAGS="$CXXFLAGS -std=c++11 -isystem external/src/SFML/include -isystem external/src/TGUI/include -Wl,-rpath=/usr/local/lib"
LDFLAGS="$LDFLAGS -Lexternal/src/SFML/lib -Lexternal/src/TGUI/lib"
LIBS="$LIBS -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -ltgui"

AC_CONFIG_FILES([Makefile docs/Makefile])

AM_CONDITIONAL([HAVE_DOXYGEN], 
[test -n "$DOXYGEN"])
AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([docs/Doxyfile])])

AC_OUTPUT
