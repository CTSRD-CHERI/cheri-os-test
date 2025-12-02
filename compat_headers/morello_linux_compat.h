/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 1982, 1986, 1989, 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

// We don't have CheriBSD's machine/cherireg.h
#define CHERI_PERM_GLOBAL  __CHERI_CAP_PERMISSION_GLOBAL__
#define CHERI_PERM_SEAL    __CHERI_CAP_PERMISSION_PERMIT_SEAL__
#define CHERI_PERM_EXECUTE __CHERI_CAP_PERMISSION_PERMIT_EXECUTE__
#define CHERI_PERM_LOAD    __CHERI_CAP_PERMISSION_PERMIT_LOAD__
#define CHERI_PERM_STORE   __CHERI_CAP_PERMISSION_PERMIT_STORE__
#define CHERI_PERM_UNSEAL  __CHERI_CAP_PERMISSION_PERMIT_UNSEAL__

#define LIBBSD_NETBSD_VIS 1
// This was taken from CheriBSD's sys/param.h
#define	nitems(x)	(sizeof((x)) / sizeof((x)[0]))
#define	PAGE_SIZE	getpagesize()
#define PAGE_SHIFT	(__builtin_ctzl(PAGE_SIZE))
#define trunc_page(x)	__align_down(x, PAGE_SIZE)
#define round_page(x)	__align_up(x, PAGE_SIZE)
