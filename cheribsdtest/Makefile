.include <src.opts.mk>

OS= uname -s

#XXX: Commented out tests that haven't been ported yet
#.if !${MACHINE_ABI:Mpurecap} || ${MK_LIB64} == "yes"
#SUBDIR+=	hybrid \
		hybrid-dynamic \
		hybrid-dynamic-mt \
		hybrid-mt
#.endif

.if (${MACHINE_ABI:Mpurecap} && !${MACHINE_ABI:Mbenchmark}) || ${MK_LIB64C} == "yes"
SUBDIR+=	purecap \
		purecap-dynamic
		#purecap-dynamic-mt \
		purecap-mt

#.if ${MACHINE_CPUARCH} == "aarch64" && ${MACHINE_ABI:Mpurecap}
#SUBDIR+=	mt-c18n
#.endif
.endif

#.if (${MACHINE_ABI:Mpurecap} && ${MACHINE_ABI:Mbenchmark}) || ${MK_LIB64CB} == "yes"
#SUBDIR+=	purecap-benchmark \
		purecap-benchmark-dynamic \
		purecap-benchmark-dynamic-mt \
		purecap-benchmark-mt

#.if ${MACHINE_CPUARCH} == "aarch64" && ${MACHINE_ABI:Mpurecap}
#SUBDIR+=	benchmark-mt-c18n
#.endif
#.endif

SUBDIR+=	malloc_early_constructor \
		libcheribsdtest_dynamic

.if ${OS} == "FreeBSD"
SUBDIR+=	malloc_revoke_enabled
.endif

SUBDIR_PARALLEL=

.include <bsd.subdir.mk>
