# Process this file with autoconf to produce a configure script.
AC_INIT(SocNetV, 0.70, dimitris.kalamaras@gmail.com, socnetv)

AC_CONFIG_SRCDIR(src/mainwindow.cpp)
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES(Makefile socnetv.pro socnetv.spec socnetv.desktop)

# Set default installation prefix.
AC_PREFIX_DEFAULT(/usr/local)
ac_prefix=$prefix
if test "x$ac_prefix" = "xNONE"; then
    ac_prefix=$ac_default_prefix
fi
AC_SUBST(ac_prefix)
AC_DEFINE_UNQUOTED(CONFIG_PREFIX, ["$ac_prefix"], [Default installation prefix.])

# Enable debugging argument option.
AC_ARG_ENABLE(debug,
  AC_HELP_STRING([--enable-debug], [enable debugging (default=no)]),
  [ac_debug="$enableval"])

if test "x$ac_debug" = "xyes"; then
   AC_DEFINE(CONFIG_DEBUG, 1, [Define if debugging is enabled.])
   ac_debug="debug"
else
   ac_debug="release"
fi
AC_SUBST(ac_debug)



# Enable SSE optimization.
AC_ARG_ENABLE(sse,
  AC_HELP_STRING([--enable-sse], [enable SSE optimization (default=yes)]),
  [ac_sse="$enableval"],
  [ac_sse="yes"])


# Standard installation base dirs.
ac_with_paths="/usr /usr/local"

# Some a-la-debian alternatives...
for X in /usr/lib /usr/share; do
  for Y in qt qt4; do
    if test -d $X/$Y/bin; then
      ac_with_paths="$ac_with_paths $X/$Y"
    fi
  done
done


# Set for alternate Qt installation dir.
AC_ARG_WITH(qt,
  AC_HELP_STRING([--with-qt=PATH], [use alternate Qt install path]),
  [ac_with_paths="$ac_with_paths $withval"])



# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_GCC_TRADITIONAL

# Checks for languages.
AC_LANG_C
AC_LANG_CPLUSPLUS

# Prepend alternate dependencies paths.
ac_path=$PATH
for X in $ac_with_paths; do
  if test -d $X/bin; then
    ac_path="$X/bin:$ac_path"
  fi
  if test -x $X/qmake; then
    ac_path="$X:$ac_path"
  fi
  if test -d $X/include; then
    for Y in qt qt4; do
      if test -d $X/include/$Y; then
        CFLAGS="-I$X/include/$Y $CFLAGS"
        CPPFLAGS="-I$X/include/$Y $CPPFLAGS"
        ac_incpath="$X/include/$Y $ac_incpath"
      fi
    done
    CFLAGS="-I$X/include $CFLAGS"
    CPPFLAGS="-I$X/include $CPPFLAGS"
    ac_incpath="$X/include $ac_incpath"
  fi
  if test -d $X/lib64; then
    LIBS="-L$X/lib64 $LIBS"
    ac_libs="-L$X/lib64 $ac_libs"
  fi
  if test -d $X/lib; then
    LIBS="-L$X/lib $LIBS"
    ac_libs="-L$X/lib $ac_libs"
  fi
done


# Check for proper Qt version.
AC_CACHE_CHECK([for Qt library version >= 4.4],
   ac_qtlib_version, [
   AC_TRY_LINK([#include "Qt/qglobal.h"], [
      #if QT_VERSION < 0x040400
      #error Qt library 4.4 or greater required.
      #endif
   ], ac_qtlib_version="yes", [
      echo "no; Qt 4.4 or greater is required"
      exit 1
   ])
])


# Check for IEEE 32bit float optimizations.
AC_CACHE_CHECK([for IEEE 32bit float optimizations],
   ac_float32, [
   AC_TRY_RUN([
      int main() { return (sizeof(float) == 4 ? 0 : 1); }
   ], ac_float32="yes", ac_float32="no")
])
if test "x$ac_float32" = "xyes"; then
   AC_DEFINE(CONFIG_FLOAT32, 1, [Define if IEEE 32bit float optimizations are enabled.])
fi

# Check for SSE optimization.
if test "x$ac_sse" = "xyes"; then
   ac_sse_cflags="-msse -mfpmath=sse -ffast-math"
   CFLAGS="$ac_sse_cflags $CFLAGS"
   CPPFLAGS="$ac_sse_cflags $CPPFLAGS"
   AC_CACHE_CHECK([for SSE optimization],
      ac_sse_check, [
      AC_TRY_RUN([
         #include <xmmintrin.h>
         #if !defined(__SSE__)
         #error SSE optimization disabled.
         #endif
         int main() {
             unsigned int a, b, c, d;
             __asm__ __volatile__
             ("cpuid": "=a" (a), "=b" (b), "=c" (c), "=d" (d) : "a" (1));
            return ((d & (1 << 25)) ? 0 : 1);
         }
      ], ac_sse_check="yes", ac_sse_check="no")
   ])
   ac_sse=$ac_sse_check
   if test "x$ac_sse" = "xyes"; then
      ac_cflags="$ac_sse_cflags $ac_cflags"
   fi
fi

AC_SUBST(ac_cflags)

# A common error message:
ac_errmsg="not found in current PATH. Maybe QT development environment isn't available (qt-devel)."

# Check for Qt qmake utility.
AC_PATH_PROG(ac_qmake, qmake, [no], $ac_path)
if test "x$ac_qmake" = "xno"; then
   AC_MSG_ERROR([qmake $ac_errmsg])
fi
AC_SUBST(ac_qmake)

# Check for Qt moc utility.
AC_PATH_PROG(ac_moc, moc, [no], $ac_path)
if test "x$ac_moc" = "xno"; then
   AC_MSG_ERROR([moc $ac_errmsg])
fi
AC_SUBST(ac_moc)

# Check for Qt uic utility.
AC_PATH_PROG(ac_uic, uic, [no], $ac_path)
if test "x$ac_uic" = "xno"; then
   AC_MSG_ERROR([uic $ac_errmsg])
fi
AC_SUBST(ac_uic)


# Checks for libraries.
AC_CHECK_LIB(m, main)
AC_CHECK_LIB(X11, main)
AC_CHECK_LIB(Xext, main)


# Checks for header files.
#AC_HEADER_STDC
#AC_HEADER_SYS_WAIT
#AC_CHECK_HEADERS(fcntl.h sys/ioctl.h sys/stat.h unistd.h signal.h)

AC_SUBST(ac_incpath)


# Checks for typedefs, structures, and compiler characteristics.
# AC_C_CONST

# Checks for library functions.
AC_CHECK_FUNCS(system)

# Finally produce a configure header file and the makefiles.
AC_OUTPUT

# make clean > /dev/null 2>&1

# Output summary message

echo
echo "  $PACKAGE_NAME $PACKAGE_VERSION"
echo
echo "  Build target . . . . . . . . . . . . . . . . . . .: $ac_debug"
echo
echo "  IEEE 32bit float optimizations . . . . . . . . . .: $ac_float32"
echo "  SSE optimization support (x86) . . . . . . . . . .: $ac_sse"
echo
echo "  Install prefix . . . . . . . . . . . . . . . . . .: $ac_prefix"
echo
echo "Now type 'make', followed by 'make install' as root."
echo



