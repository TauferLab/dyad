#########################
# Autoconf requirements #
#########################
# Adds a minimum required Autoconf version to be consistent with flux-sched
AC_PREREQ([2.63])

################
# Package Info #
################
# Initialize the "dyad" package with a version derived from git
AC_INIT([dyad],
        m4_esyscmd([git describe --always --tags | awk '/.*/ {sub(/^v/, ""); printf "%s",$1; exit}']))
# Config directory is set to "./config"
AC_CONFIG_AUX_DIR([config])
# Macro directory is set to "./config"
AC_CONFIG_MACRO_DIR([config])
# Src directory is set to "."
AC_CONFIG_SRCDIR([README.md])

#######################
# Initialize Automake #
#######################
AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_SILENT_RULES([yes])

######################
# Initialize Libtool #
######################
LT_INIT([shared dlopen])

###################################################
# Set the prefix to the prefix of Flux by default #
###################################################
# If the "flux" program is in PATH, set the default prefix to be
# the parent of the directory containing "flux" (i.e., the flux-core prefix).
# If the "flux" program is not found, use the value of AC_PREFIX_DEFAULT (usually /usr/local)
# as the default prefix.
# In either case, the prefix can be overwritten with the "--prefix" flag.
AC_PREFIX_PROGRAM([flux])

############################################
# Checks for programs and adds definitions #
############################################
# Define _GNU_SOURCE to make sure we get all the prototypes we need
AC_DEFINE([_GNU_SOURCE], 1,
          [Define _GNU_SOURCE so that we get all necessary prototypes])
# Locate "awk" (might not be needed, but added during autogeneration of configury)
AC_PROG_AWK
# Locate the C++ compiler
AC_PROG_CXX
# Ensure that the C++ compiler supports C++11 (not gnu++11)
AX_CXX_COMPILE_STDCXX([11], [noext], [mandatory])
# Locate the C compiler and add flags to ensure C99 is enabled
AC_PROG_CC_C99
AC_PROG_INSTALL
# Check if system supports "ln -s"
AC_PROG_LN_S
# Set the path to Make
AC_PROG_MAKE_SET
# Ensure the C compiler can use the "-c" and "-o" flags at the same time
AM_PROG_CC_C_O
# Load pkg-config macros from ./config/pkg.m4
PKG_PROG_PKG_CONFIG

#################################################
# Checks for additional configure-time features #
#################################################
# Add flag to enable DYAD's debug functionality
AC_ARG_ENABLE([dyad-debug],
    [AS_HELP_STRING([--enable-dyad-debug],
                    [whether to include debugging prints in DYAD])],
    [enable_debug=$enableval],
    [enable_debug=no]
)
# Add flag to enable building of URPC
AC_ARG_ENABLE([urpc],
    [AS_HELP_STRING([--enable-urpc],
                    [enable compilation and install of URPC])],
    [enable_urpc=$enableval],
    [enable_urpc=no]
)
# Add flag to enable PerfFlow Aspect (PFA) support
AC_ARG_ENABLE([perfflow],
    [AS_HELP_STRING([--enable-perfflow],
                    [enable performance measurement with PerfFlow Aspect])],
    [enable_perfflow=$enableval],
    [enable_perfflow=no]
)

########################
# Checks for libraries #
########################
# Check for the dl library (specifically, the "dlsym" function; Required)
AC_CHECK_LIB([dl], [dlsym])
# Check for UCX v1.6.0 or higher (Required)
PKG_CHECK_MODULES([UCX],
    [ucx >= 1.6.0],
    [],
    []
)
# Check for flux-core (Required)
AX_FLUX_CORE
# Check for Jansson (Optional, but must be installed in flux-core is installed)
PKG_CHECK_MODULES([JANSSON],
    [jansson >= 2.10],
    [pkg_check_jansson_found=yes],
    [pkg_check_jansson_found=no]
)
# If URPC is to be built, Jansson is required
# So, produce error if URPC is enabled and Jansson isn't found
if test "x$enable_urpc" = "xyes" && test "x$pkg_check_jansson_found" = "xno"; then
    AC_MSG_ERROR([jansson cannot be located, but it is required when '--enable-urpc' is provided])
fi
# Set "URPC" in Automake based on the configury's "enable_urpc" flag
AM_CONDITIONAL([URPC], [test "x$enable_urpc" = "xyes"])
# Check for PerfFlow Aspect (PFA; Optional)
AX_PERFFLOW_ASPECT([PERFFLOW],
    [pkg_check_perfflow_found=yes],
    [pkg_check_perfflow_found=no]
)
# If PFA support was enabled, PFA is required
# If PFA isn't found in this case, raise an error
if test "x$enable_perfflow" = "xyes" && test "x$pkg_check_perfflow_found" = "xno"; then
    AC_MSG_ERROR([requested PerfFlow Aspect support, but cannot find PerfFlow Aspect with pkg-config])
fi
# Set "PERFFLOW" in Automake based on the configury's "enable_perfflow" flag
AM_CONDITIONAL([PERFFLOW], [test "x$enable_perfflow" = "xyes"])

###########################
# Checks for header files #
###########################
# Check for C standard library headers
AC_HEADER_STDC
# Check for additional necessary headers
AC_CHECK_HEADERS( \
    fcntl.h \
    stdint.h \
    [sys/param.h] \
    unistd.h \
)

#################################################################
# Checks for typedefs, structures, and compiler characteristics #
#################################################################
# Confirm C compiler supports booleans
AC_CHECK_HEADER_STDBOOL
# Check if C compiler supports the "inline" keyword
AC_C_INLINE
# Ensure the C compiler and standard library support the following types:
#   * int8_t
#   * mode_t
#   * size_t
#   * ssize_t
#   * uint32_t
#   * uint64_t
#   * uint8_t
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
# Defining _POSIX_C_SOURCE for now
# TODO look into side effects of setting this on non-Linux OSes (e.g., macOS)
AC_DEFINE([_POSIX_C_SOURCE], 200112L, [Set the version of the POSIX standard to use])

################################
# Checks for library functions #
################################
# Ensure that the "fork" and "vfork" functions are available
AC_FUNC_FORK
# Ensure that the "malloc" function exists and is compatible with GNU C's malloc
AC_FUNC_MALLOC
# Ensure that the "mmap" function exists and works correctly
AC_FUNC_MMAP
# Ensure that the "realloc" function exists and is compatible with GNU C's realloc
AC_FUNC_REALLOC
# Check other necessary functions
AC_CHECK_FUNCS( \
    clock_gettime \
    getcwd \
    memset \
    mkdir \
    realpath \
    regcomp \
    setenv \
    strerror \
)

#######################################
# Add any necessary compilation flags #
#######################################
if test "x$enable_debug" = xyes; then
    CPPFLAGS="$CPPFLAGS -DDYAD_FULL_DEBUG=1 -DDYAD_LOGGING_ON=1"
fi

########################
# Configures Makefiles #
########################
AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/utils/Makefile
                 src/utils/libtap/Makefile
                 src/core/Makefile
                 src/stream/Makefile
                 src/modules/Makefile
                 src/wrapper/Makefile])

#####################
# Complete Autoconf #
#####################
AC_OUTPUT
