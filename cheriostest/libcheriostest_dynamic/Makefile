.include <src.opts.mk>

# Don't pass -S because GNU install doesn't understand it
NO_SAFE_LIBINSTALL=	yes
SHLIB=	cheriostest_dynamic
SHLIB_MAJOR=	0

SHLIBDIR= /usr/local/lib/
# No manpage; this is internal.
MAN=
# Don't create links
NO_LIB_LINKS=

SRCS=	cheriostest_dynamic_fptr.c					\
	cheriostest_dynamic_identity_cap.c				\
	cheriostest_dynamic_tls.c

.if ${TARGET_OS} == "FreeBSD"
SRCS+=	cheriostest_dynamic_ifunc.c
.endif

MK_DEBUG_FILES=	"no"

.include <bsd.lib.mk>
