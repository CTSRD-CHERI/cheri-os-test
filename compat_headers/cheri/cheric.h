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

#include <sys/cdefs.h>
#include <sys/types.h>
#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <stdbool.h>
#include <stdint.h>
#endif

#include <cheriintrin.h>

#if __has_feature(capabilities)
#include "../morello_linux_compat.h"

/*
 * Get the top of a capability (i.e. one byte past the last accessible one)
 * XXXPM: Is cheri_gettop() equivalent to cheri_high_get() in cheriintrin.h?
 */
#define	cheri_gettop(cap)	__extension__({			\
	__typeof__(cap) c = (cap);				\
	(cheri_base_get(c) + cheri_length_get(c));			\
})

#define cheri_ptr(ptr, len)	\
	cheri_bounds_set(    \
	    (__cheri_tocap __typeof__((ptr)[0]) *__capability)ptr, len)

#if defined(__aarch64__)
#define cheri_ptrperm(ptr, len, perm)	\
	cheri_perms_and(cheri_ptr(ptr, len), perm | CHERI_PERM_GLOBAL)
#elif defined(__riscv_zcheripurecap)
#define cheri_ptrperm(ptr, len, perm)	\
	cheri_perms_and(cheri_ptr(ptr, len), perm)
#endif
#endif	/* __has_feature(capabilities) */

/* Turn on the checking by default for now (until we have fixed everything)*/
/*#define __check_low_ptr_bits_assignment
#ifdef __check_low_ptr_bits_assignment
#ifndef _cheri_bits_assert
#define _cheri_bits_assert(e) assert(e)
#endif

#define __runtime_assert_sensible_low_bits(bits)                               \
  __extension__({                                                              \
    _cheri_bits_assert((bits) < 32 && "Should only use the low 5 pointer bits"); \
    bits;                                                                      \
  })
#else
#define __runtime_assert_sensible_low_bits(bits) bits
#endif
#define __static_assert_sensible_low_bits(bits)                                \
  __extension__({                                                              \
    _Static_assert((bits) < 32, "Should only use the low 5 pointer bits");     \
    bits;                                                                      \
  })*/

/* Provide macros to make it easier to work with the raw CRAM/CRRL results: */
#define	CHERI_REPRESENTABLE_ALIGNMENT(len) \
	(~cheri_representable_alignment_mask(len) + 1)
#define	CHERI_REPRESENTABLE_ALIGN_DOWN(base, len) \
	((base) & cheri_representable_alignment_mask(len))

#define	CHERI_ALIGN_MASK(l)		~(cheri_representable_alignment_mask(l))

#endif /* _SYS_CHERIC_H_ */
// CHERI CHANGES START
// {
//   "updated": 20230509,
//   "target_type": "header",
//   "changes": [
//     "support",
//     "ctoptr"
//   ],
//   "changes_purecap": [
//     "support"
//   ]
// }
// CHERI CHANGES END
