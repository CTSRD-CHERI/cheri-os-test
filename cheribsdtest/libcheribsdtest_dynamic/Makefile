.include <src.opts.mk>

OS!=	uname -s

# Don't pass -S because GNU install doesn't understand it
NO_SAFE_LIBINSTALL=	yes
SHLIB=	cheribsdtest_dynamic
SHLIB_MAJOR=	0
#Don't install in usr/lib/
SHLIBDIR=
# No manpage; this is internal.
MAN=
# Don't create links
NO_LIB_LINKS=

SRCS=	cheribsdtest_dynamic_fptr.c					\
	cheribsdtest_dynamic_identity_cap.c				\
	cheribsdtest_dynamic_tls.c

.if ${MACHINE_CPUARCH} == "aarch64" && ${OS} == "FreeBSD"
# XXXPM: Why the CPUARCH condition?
SRCS+=	cheribsdtest_dynamic_ifunc.c
.endif

.if ${MACHINE_CPUARCH} == "aarch64"
_RESOURCE_DIR=	\
	${SRCTOP}/build/morello-compiler-rt-builtins-linux-morello-purecap-build
.elif ${MACHINE_CPUARCH} == "rv64imafdczcherihybrid_zcherilevels"
_RESOURCE_DIR=	\
	${SRCTOP}/build/cheri-std093-compiler-rt-builtins-linux-riscv64-purecap-std093-build
.endif
LDFLAGS+=	-rtlib=compiler-rt \
	-resource-dir=${_RESOURCE_DIR}
MK_DEBUG_FILES=	"no"

.include <bsd.lib.mk>
