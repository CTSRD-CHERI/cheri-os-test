.include <src.opts.mk>

OS!=	uname -s

SUBDIR=		purecap \
		purecap-dynamic \
		purecap-dynamic-mt \
		purecap-mt

.if ${MACHINE_ABI:Mpurecap} || ${MK_LIB64C} == "yes"
.if ${MACHINE_CPUARCH} == "aarch64"
SUBDIR+=	mt-c18n
.endif
.endif

#.if ${OS} == "FreeBSD"
# XXXPM: Do we have a benchmarking ABI on Linux?
#.if (${MACHINE_ABI:Mpurecap} && ${MACHINE_ABI:Mbenchmark}) || ${MK_LIB64CB} == "yes"
#SUBDIR+=	purecap-benchmark \
		purecap-benchmark-dynamic \
		purecap-benchmark-dynamic-mt \
		purecap-benchmark-mt
#.endif
#.endif

.include <bsd.subdir.mk>
