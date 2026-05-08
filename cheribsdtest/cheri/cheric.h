/*-
 * Copyright (c) 2013-2016 Robert N. M. Watson
 * Copyright (c) 2021 Microsoft Corp.
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYS_CHERIC_H_
#define	_SYS_CHERIC_H_

#include <cheriintrin.h>

/*
 * Get the top of a capability (i.e. one byte past the last accessible one)
 * XXXPM: Is cheri_gettop() equivalent to cheri_high_get() in cheriintrin.h?
 */
#define	cheritest_cheri_gettop(cap)	__extension__({			\
	__typeof__(cap) c = (cap);				\
	(cheri_base_get(c) + cheri_length_get(c));			\
})

#define cheritest_cheri_ptr(ptr, len)	\
	cheri_bounds_set(    \
	    (__cheri_tocap __typeof__((ptr)[0]) *__capability)ptr, len)

#if defined(__aarch64__)
#define cheritest_cheri_ptrperm(ptr, len, perm)	\
	cheri_perms_and(cheritest_cheri_ptr(ptr, len), perm | CHERI_PERM_GLOBAL)
#elif defined(__riscv_zcheripurecap)
#define cheritest_cheri_ptrperm(ptr, len, perm)	\
	cheri_perms_and(cheritest_cheri_ptr(ptr, len), perm)
#endif

/* Provide macros to make it easier to work with the raw CRAM/CRRL results: */
#define	CHERITEST_CHERI_REPRESENTABLE_ALIGNMENT(len) \
	(~cheri_representable_alignment_mask(len) + 1)

#define	CHERITEST_CHERI_ALIGN_MASK(l)		~(cheri_representable_alignment_mask(l))

#endif /* _SYS_CHERIC_H_ */
