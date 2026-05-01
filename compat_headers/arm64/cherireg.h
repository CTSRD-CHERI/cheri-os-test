/*-
 * Copyright (c) 2011-2018 Robert N. M. Watson
 * Copyright (c) 2016-2020 Andrew Turner
 * Copyright (c) 2020 John Baldwin
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * Portions of this software were developed by SRI International and
 * the University of Cambridge Computer Laboratory (Department of
 * Computer Science and Technology) under DARPA contract
 * HR0011-18-C-0016 ("ECATS"), as part of the DARPA SSITH research
 * programme.
 *
 * This work was supported by Innovate UK project 105694, "Digital Security
 * by Design (DSbD) Technology Platform Prototype".
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
#ifndef _ARM64_INCLUDE_CHERIREG_H_
#define	_ARM64_INCLUDE_CHERIREG_H_

/*
 * CHERI ISA-defined constants for capabilities -- suitable for inclusion from
 * assembly source code.
 */
#define	CHERI_PERM_SW0				(1 << 2)	/* 0x00000004 */
#define	CHERI_PERM_SW1				(1 << 3)	/* 0x00000008 */
#define	CHERI_PERM_SW2				(1 << 4)	/* 0x00000010 */
#define	CHERI_PERM_SW3				(1 << 5)	/* 0x00000020 */

/*
 * Macros defining initial permission sets:
 *
 * CHERI_PERMS_SWALL: Mask of all available software-defined permissions
 * CHERI_PERMS_HWALL: Mask of all available hardware-defined permissions
 */
#define	CHERI_PERMS_SWALL						\
	(CHERI_PERM_SW0 | CHERI_PERM_SW1 | CHERI_PERM_SW2 |		\
	CHERI_PERM_SW3)

/*
 * Basic userspace permission mask; CHERI_PERM_EXECUTE will be added for
 * executable capabilities (pcc); CHERI_PERM_STORE, CHERI_PERM_STORE_CAP,
 * and CHERI_PERM_STORE_LOCAL_CAP will be added for data permissions (ddc).
 */
#define	CHERI_PERMS_USERSPACE						\
	(CHERI_PERM_GLOBAL | CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP |	\
	CHERI_PERM_INVOKE |					\
	(CHERI_PERMS_SWALL & ~CHERI_PERM_SW_VMEM))

#define	CHERI_PERMS_USERSPACE_CODE					\
	(CHERI_PERMS_USERSPACE | CHERI_PERM_EXECUTE |			\
	ARM_CAP_PERMISSION_EXECUTIVE | CHERI_PERM_LOAD_MUTABLE)

#define	CHERI_PERMS_USERSPACE_SEALCAP					\
	(CHERI_PERM_GLOBAL | CHERI_PERM_SEAL | CHERI_PERM_UNSEAL)

#define CHERI_PERMS_USERSPACE                                           \
	(CHERI_PERM_GLOBAL | CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP |    \
	CHERI_PERM_INVOKE |                                 \
	(CHERI_PERMS_SWALL & ~CHERI_PERM_SW_VMEM))

#define CHERI_PERMS_USERSPACE_DATA                                      \
	(CHERI_PERMS_USERSPACE | CHERI_PERM_STORE |                     \
	CHERI_PERM_STORE_CAP | CHERI_PERM_STORE_LOCAL_CAP |             \
	CHERI_PERM_LOAD_MUTABLE)

#define	CHERI_CAP_USER_DATA_PERMS	CHERI_PERMS_USERSPACE_DATA

/*
 * The CHERI object-type space is split between userspace and kernel,
 * permitting kernel object references to be delegated to userspace (if
 * desired).  Currently, we provide 13 bits of namespace to each, with the top
 * bit set for kernel object types, but it is easy to imagine other splits.
 * User and kernel software should be written so as to not place assumptions
 * about the specific values used here, as they may change.
 *
 * On Morello otype 0 is unsealed, and 1-3 are reserved.
 */
#define	CHERI_OTYPE_BITS	(14)
#define	CHERI_OTYPE_USER_MIN	(4)
#define	CHERI_OTYPE_USER_MAX	((1 << (CHERI_OTYPE_BITS - 1)) - 1)

#endif /* _ARM64_INCLUDE_CHERIREG_H_ */
