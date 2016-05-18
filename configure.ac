AC_PREREQ(2.59)
AC_INIT(zm,2.0.0a,support@zoneminder.com,ZoneMinder,http://www.zoneminder.com/downloads.html)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR(src/zm.h)
AM_CONFIG_HEADER(config.h)

PATH_BUILD=`pwd`
AC_SUBST(PATH_BUILD)
TIME_BUILD=`date +'%s'`
AC_SUBST(TIME_BUILD)

AC_PREFIX_DEFAULT([/opt/])
CPPFLAGS="-D__STDC_CONSTANT_MACROS"

AC_ARG_VAR(ZM_USER,[Name of ZoneMinder user, zoneminder])
if test "$ZM_USER" == ""; then
    AC_SUBST(ZM_USER,[zoneminder])
fi
AC_ARG_VAR(ZM_WEB_DIR,[Location to install web GUI html files, default /opt/zm2/web/html])
if test "$ZM_WEB_DIR" == ""; then
    AC_SUBST(ZM_WEB_DIR,$prefix/web/html)
fi
AC_ARG_VAR(ZM_CGI_DIR,[Location to install web GUI cgi files, default /opt/zm2/web/cgi-bin])
if test "$ZM_CGI_DIR" == ""; then
    AC_SUBST(ZM_CGI_DIR,$prefix/web/cgi-bin)
fi
AC_ARG_VAR(ZM_WEB_HOST,[Hostname for virtual hosting, default current host name])
if test "$ZM_WEB_HOST" == ""; then
    AC_SUBST(ZM_WEB_HOST,`hostname`)
fi
AC_ARG_VAR(ZM_DB_HOST,[Hostname where ZoneMinder database located, default localhost])
if test "$ZM_DB_HOST" == ""; then
    AC_SUBST(ZM_DB_HOST,[localhost])
fi
AC_ARG_VAR(ZM_DB_NAME,[Name of ZoneMinder database, default zm])
if test "$ZM_DB_NAME" == ""; then
    AC_SUBST(ZM_DB_NAME,[zm])
fi
AC_ARG_VAR(ZM_DB_USER,[Name of ZoneMinder database user, default zmuser])
if test "$ZM_DB_USER" == ""; then
    AC_SUBST(ZM_DB_USER,[zmuser])
fi
AC_ARG_VAR(ZM_DB_PASS,[Password of ZoneMinder database user, default zmpass])
if test "$ZM_DB_PASS" == ""; then
    AC_SUBST(ZM_DB_PASS,[zmpass])
fi
AC_ARG_VAR(ZM_SSL_LIB,[Library to use for ssl functions, default gnutls])
if test "$ZM_SSL_LIB" == ""; then
    AC_SUBST(ZM_SSL_LIB,gnutls)
fi
AC_ARG_VAR(ZM_MYSQL_ENGINE,[MySQL engine to use with database, default MyISAM])
if test "$ZM_MYSQL_ENGINE" == ""; then
    AC_SUBST(ZM_MYSQL_ENGINE,MyISAM)
fi
AC_ARG_VAR(ZM_RUNDIR,[Location of transient process files, default /var/run/zm])
if test "$ZM_RUNDIR" == ""; then
    AC_SUBST(ZM_RUNDIR,[/var/run/zm])
fi
AC_ARG_VAR(ZM_TMPDIR,[Location of temporary files, default /tmp/zm])
if test "$ZM_TMPDIR" == ""; then
    AC_SUBST(ZM_TMPDIR,[/tmp/zm])
fi
AC_ARG_VAR(ZM_LOGDIR,[Location of generated log files, default /var/log/zm])
if test "$ZM_LOGDIR" == ""; then
    AC_SUBST(ZM_LOGDIR,[/var/log/zm])
fi
AC_ARG_VAR(ZM_LIBARCH,[Architecture specific library path, default lib, try lib64 on 64 bit systems])
if test "$ZM_LIBARCH" == ""; then
    AC_SUBST(ZM_LIBARCH,[/lib])
fi

MYSQL_PREFIX=/usr
AC_ARG_WITH(mysql,
    [  --with-mysql=<path>        prefix of MySQL installation, default /usr],
    [MYSQL_PREFIX=$with_mysql],
    AC_MSG_WARN([You can call configure with the --with-mysql option.
    This tells configure where to find the MySql C library and headers if configure cannot
    locate them automatically.
    e.g. --with-mysql=/usr/local or --with-mysql=/usr])
)
AC_SUBST(MYSQL_PREFIX)
MYSQL_LIBS="-L${MYSQL_PREFIX}/${ZM_LIBARCH}/mysql"
MYSQL_CFLAGS="-I${MYSQL_PREFIX}/include"
AC_SUBST(MYSQL_LIBS)
AC_SUBST(MYSQL_CFLAGS)

LDFLAGS="${MYSQL_LIBS} $LDFLAGS"

FFMPEG_PREFIX=/usr/local
AC_ARG_WITH(ffmpeg,
    [  --with-ffmpeg=<path>       prefix of ffmpeg root directory for libavcodec etc, default /usr/local],
    [FFMPEG_PREFIX=$with_ffmpeg],
    AC_MSG_WARN([You can call configure with the --with-ffmpeg option.
    This tells configure where to find the ffmpeg root directory within which are the libavcodec 
    and libavformat files that are used for various functions in ZoneMinder. Ensure that your copy of
    ffmpeg has installed libraries and headers as well as binaries.
    e.g. --with-ffmpeg=/opt/ffmpeg])
)
AC_SUBST(FFMPEG_PREFIX)
FFMPEG_LIBS="-L${FFMPEG_PREFIX}/${ZM_LIBARCH}"
FFMPEG_CFLAGS="-I${FFMPEG_PREFIX}/include"
AC_SUBST(FFMPEG_LIBS)
AC_SUBST(FFMPEG_CFLAGS)

LDFLAGS="${FFMPEG_LIBS} $LDFLAGS"
CFLAGS="${FFMPEG_CFLAGS} $CFLAGS"
CPPFLAGS="${FFMPEG_CFLAGS} $CPPFLAGS"

ENABLE_V4L=yes
AC_ARG_ENABLE(video4linux,
    [  --enable-video4linux=<yes|no> enable or disabled video4linux support, default enabled],
    [ENABLE_V4L=$enable_video4linux],
    AC_MSG_WARN([[You can call configure with the --enable-video4linux=<yes|no> or --disable-video4linux option.
    This tells configure whether to compile ZoneMinder with Video4Linux support. Video4Linux is used
    to communicate with cameras that are directly connected to your system, normally using analog or
    USB video. If you are installing ZoneMinder on a system that only uses network cameras then you 
    can disable this support if the Video4Linux supports files are not present. Leaving it enabled
    has no performance penalty if it not being used.
    e.g. --enable-video4linux=yes or --disable-video4linux]])
)
if test "$ENABLE_V4L" = "yes"; then
    AC_DEFINE(ZM_V4L,1,"Whether to support the Video4Linux interface")
fi

ENABLE_MMAP=yes
AC_ARG_ENABLE(mmap,
    [  --enable-mmap=<yes|no> enable or disabled mapped memory versus shared memory, default enabled],
    [ENABLE_MMAP=$enable_mmap],
    AC_MSG_WARN([[You can call configure with the --enable-mmap=<yes|no> or --disable-mmap option.
    This tells configure whether to compile ZoneMinder with mmap support rather than IPC shared
    memory. This is a new feature that uses memory mapped into files which all processes can share.
    Memory mapping requires less configuration and is more flexible than shared memory but may
    slow down your system unless the mapped files are configured to reside on a fast or RAM based
    filesystem however this will not normally be the case.
    e.g. --enable-mmap=yes or --disable-mmap]])
)
if test "$ENABLE_MMAP" = "yes"; then
    AC_DEFINE(ZM_MEM_MAPPED,1,"Whether to use mapped rather than shared memory")
fi

ENABLE_DEBUG=yes
AC_ARG_ENABLE(debug,
    [  --enable-debug=<yes|no>    enable or disable debug, default enabled],
    [ENABLE_DEBUG=$enable_debug],
    AC_MSG_WARN([[You can call configure with the --enable-debug=<yes|no> or --disable-debug option.
    This tells configure whether to compile ZoneMinder with debug included. Although debug is included
    by default it is not output unless explicitly switched on elsewhere. These checks may induce a
    small penalty on performance and if you are after squeezing the maximum possible performance out
    of ZoneMinder you may use this switch to prevent debug from being compiled in.
    e.g. --enable-debug=yes or --disable-debug]])
)
if test "$ENABLE_DEBUG" != "yes"; then
    AC_DEFINE(ZM_DBG_OFF,1,"Whether debug is switched off and compiled out")
fi

ENABLE_CRASHTRACE=yes
AC_ARG_ENABLE(crashtrace,
    [  --enable-crashtrace=<yes|no> enable or disabled crash tracing, default enabled],
    [ENABLE_CRASHTRACE=$enable_crashtrace],
    AC_MSG_WARN([You can call configure with the --enable-crashtrace=<yes|no> or --disable-crashtrace option.
    This tells configure whether to compile ZoneMinder with crash tracing included. This allows a
    dump of the stack trace when a ZoneMinder binary crashes or is killed by an unexpected signal.
    Although this should work on most systems it does rely on un(or loosely) documented features and
    so should be regarded as experimental. If you experience problems compiling zm_signal.cpp or 
    ZoneMinder binaries fail to shut down correctly then you should probably disable this feature.
    e.g. --enable-crashtrace=yes or --disable-crashtrace])
)
if test "$ENABLE_CRASHTRACE" != "yes"; then
  AC_DEFINE(ZM_NO_CRASHTRACE,1,"Whether crash tracing is switched off and compiled out")
fi

# Compiler
AC_LANG_CPLUSPLUS

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_RANLIB
AC_PROG_MAKE_SET

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_TYPE_SIGNAL

AC_CHECK_TYPES(siginfo_t,,,[#include <signal.h>])
AC_CHECK_TYPES(struct sigcontext,,,[#include <signal.h>])
AC_CHECK_MEMBERS([struct sigcontext.eip],,,[#include <signal.h>])
AC_CHECK_TYPES(ucontext_t,,,[#include <signal.h>])

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([gethostbyname gethostname gettimeofday memmove memset mkdir munmap putenv select socket sqrt strcasecmp strchr strcspn strerror strncasecmp strrchr strsignal strspn strstr strtol strtoull])

# Other programs
AC_CHECK_PROG(OPT_FFMPEG,ffmpeg,yes,no)
AC_PATH_PROG(PATH_FFMPEG,ffmpeg)

# Checks for libraries.
AC_CHECK_LIB(mysqlclient,mysql_init,,AC_MSG_ERROR(zm requires libmysqlclient.a))
AC_CHECK_LIB(jpeg,jpeg_start_compress,,AC_MSG_ERROR(zm requires libjpeg.a))
AC_CHECK_LIB(pthread,pthread_create,,AC_MSG_ERROR(zm requires libpthread.a))
AC_CHECK_LIB(dl,dlsym,,AC_MSG_ERROR(zm requires libdl.a))
if test "$ZM_SSL_LIB" == "openssl"; then
AC_CHECK_LIB(crypto,MD5,,AC_MSG_WARN([libcrypto.a is required for authenticated streaming - use ZM_SSL_LIB option to select gnutls instead]))
else
AC_CHECK_HEADERS(gcrypt.h,,AC_MSG_WARN(zm requires libgcrypt headers to be installed for gnutls),)
AC_CHECK_LIB(gcrypt,gcry_check_version,,AC_MSG_WARN([libgcrypt.a is required for authenticated streaming - use ZM_SSL_LIB option to select openssl instead]))
AC_CHECK_LIB(gnutls-openssl,MD5,,AC_MSG_WARN([gnutls-openssl.a is required for authenticated streaming - use ZM_SSL_LIB option to select openssl instead]))
fi
AC_CHECK_LIB(z,zlibVersion)
AC_CHECK_LIB(x264,x264_predict_16x16_init)
AC_CHECK_LIB(avcore,av_image_copy,,)
AC_CHECK_LIB(avcodec,av_init_packet,,AC_MSG_WARN(libavcodec.a is required for MPEG streaming))
AC_CHECK_LIB(avformat,av_register_all,,AC_MSG_WARN(libavformat.a is required for MPEG streaming))
AC_CHECK_LIB(avdevice,avdevice_register_all,,AC_MSG_WARN(libavdevice.a may be required for MPEG streaming))
AC_CHECK_LIB(avutil,av_malloc,,AC_MSG_ERROR(libavutil.a is required, please install ffmpeg development libraries))
AC_CHECK_LIB(swscale,sws_scale,,AC_MSG_ERROR(libswscale.a is required, please install ffmpeg development libraries))
AC_CHECK_LIB(bz2,BZ2_bzCompress,,AC_MSG_ERROR([libbz2.a is required with recent versions of ffmpeg]))
AC_CHECK_LIB(z,compress,,)

# Checks for header files.
AC_FUNC_ALLOCA
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h limits.h memory.h netdb.h netinet/in.h stddef.h stdlib.h string.h strings.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h syslog.h unistd.h values.h])
AC_CHECK_HEADERS(linux/videodev2.h,,AC_MSG_WARN(zm requires Video4Linux2 to be installed for V4L2 support))
AC_CHECK_HEADERS(jpeglib.h,,AC_MSG_ERROR(zm requires libjpeg headers to be installed),)
AC_CHECK_HEADERS(mysql/mysql.h,,AC_MSG_ERROR(zm requires MySQL headers - check that MySQL development packages are installed),)
AC_CHECK_HEADERS(libavutil/avutil.h,,,)
AC_CHECK_HEADERS(libavcodec/avcodec.h,,,)
AC_CHECK_HEADERS(libavformat/avformat.h,,,)
AC_CHECK_HEADERS(libswscale/swscale.h,,,)
if test "$ENABLE_MMAP" = "yes"; then
AC_CHECK_HEADERS(sys/mman.h,,,)
AC_CHECK_HEADERS(fcntl.h,,,)
else
AC_CHECK_HEADERS(sys/ipc.h,,,)
AC_CHECK_HEADERS(sys/shm.h,,,)
fi
AC_CHECK_HEADERS(zlib.h,,,)

AC_CHECK_DECLS(round,,,[#include <math.h>])
if test "$ZM_SSL_LIB" == "openssl"; then
AC_CHECK_DECLS(MD5,,AC_MSG_ERROR([zm requires openssl/md5.h - use ZM_SSL_LIB option to select gnutls instead]),[#include <stdlib.h>
#include <openssl/md5.h>])
else
AC_CHECK_DECLS(MD5,,AC_MSG_ERROR([zm requires gnutls/openssl.h - use ZM_SSL_LIB option to select openssl instead]),[#include <stdlib.h>
#include <gnutls/openssl.h>])
fi
AC_CHECK_DECLS(backtrace,,,[#include <execinfo.h>])

AC_SUBST(LDFLAGS)

AC_DEFINE_DIR([BINDIR],[bindir],[Expanded binary directory])
AC_DEFINE_DIR([LIBDIR],[libdir],[Expanded library directory])
AC_SUBST(ZM_PID,"$ZM_RUNDIR/zm.pid")
AC_DEFINE_DIR([SYSCONFDIR],[sysconfdir],[Expanded configuration directory])
AC_SUBST(ZM_CONFIG,"$SYSCONFDIR/zm.conf")

AC_CONFIG_FILES([Makefile src/Makefile src/zmConfig.h src/libgen/Makefile src/libimg/Makefile src/consumers/Makefile src/encoders/Makefile src/processors/Makefile src/protocols/Makefile src/providers/Makefile])

# Create the definitions for compilation and defaults for the database
AC_CONFIG_COMMANDS([src/zmConfigDefines.h],[perl ./zmconfgen.pl])

AC_OUTPUT
