/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 SRI International
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory (Department of Computer Science and
 * Technology) under DARPA contract HR0011-18-C-0016 ("ECATS"), as part of the
 * DARPA SSITH research programme.
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

#include <sys/cdefs.h>

#if !__has_feature(capabilities)
#error "This code requires a CHERI-aware compiler"
#endif

#if __linux__
// This needs to be included for the definition of SEGV_CAPBOUNDSERR
#include <linux/signal.h>
// This define avoids a redefinition of sigset_t in musl's alltypes.h
#define __DEFINED_sigset_t
#endif

#include <sys/param.h>

#ifdef __FreeBSD__
#include <cheri/cheri.h>
#endif
#include <cheri/cheric.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cheribsdtest.h"

#ifdef __linux__
#include <morello_linux_compat.h>
#endif

#define	NOT_IMPL_MSG "This test hasn't been fully implemented for RISC-V yet"

#define	STR_VAL	"123"

static const char *
skip_local_global_required(const struct cheri_test *test __attribute__((__unused__)))
{
#if defined(__riscv_zcheripurecap)
	FILE *f;
	ssize_t buf_size = 4096;
	char *line = malloc(buf_size);

	f = fopen("/proc/cpuinfo", "r");
	if (f == NULL)
		cheribsdtest_failure_errx("Couldn't open /proc/cpuinfo");

	while (getline(&line, &buf_size, f) != -1) {
		if (strstr(line, "isa") != NULL) {
			if (strstr(line, "zylevels1") != NULL) {
				free(line);
				return NULL;
			}
		}
	}
	free(line);
	return ("zylevels1 required");
#else
	return NULL;
#endif
}

CHERIBSDTEST(store_local_allowed,
    "Checks local capabilities can be stored via default capabilities",
    .ct_check_skip = skip_local_global_required,)
{
	char str[] = STR_VAL;
	char * __capability cap = str;
	char * __capability target;
	char * __capability * __capability targetp = &target;

	CHERIBSDTEST_VERIFY(strcmp(STR_VAL, str) == 0);
	*targetp = cap;
	CHERIBSDTEST_VERIFY(
	    strcmp(STR_VAL, (__cheri_fromcap char *)target) == 0);

#if defined(__aarch64__)
	/* Make cap local */
	cap = cheri_perms_and(cap, ~CHERI_PERM_GLOBAL);
#elif defined(__riscv)
	cheribsdtest_failure_errx(NOT_IMPL_MSG);
#endif

	/* Store local cap through cap with store-local permission */
	*targetp = cap;
	CHERIBSDTEST_VERIFY(
	    strcmp(STR_VAL, (__cheri_fromcap char *)target) == 0);

	cheribsdtest_success();
}

#ifndef __riscv_zcherilevels
CHERIBSDTEST(store_local_disallowed,
    "Checks local capabilities can not be stored via non-store-local capabilities",
#ifdef __FreeBSD__
    .ct_flags = CT_FLAG_SIGNAL | CT_FLAG_SI_CODE | CT_FLAG_SI_TRAPNO,
    .ct_signum = SIGPROT,
    .ct_si_code = SI_CODE_STORELOCAL,
    .ct_si_trapno = TRAPNO_LOAD_STORE,
#elif defined(__linux__)
    .ct_flags = CT_FLAG_SIGNAL | CT_FLAG_SI_CODE | CT_FLAG_SI_TRAPNO,
    .ct_signum = SIGSEGV,
    .ct_si_code = SEGV_CAPPERMERR,
#endif
)
#else
CHERIBSDTEST(store_local_disallowed,
    "Checks tag is stripped when local capabilities are stored via non-store-local capabilities")
#endif
{
	char str[] = STR_VAL;
	char * __capability cap = str;
	char * __capability volatile target;
	char * __capability volatile * __capability targetp = &target;

	CHERIBSDTEST_VERIFY(strcmp(STR_VAL, str) == 0);
	*targetp = cap;
	CHERIBSDTEST_VERIFY(
	    strcmp(STR_VAL, (__cheri_fromcap char *)target) == 0);

	/* Make cap local */
	cap = cheri_perms_and(cap, ~CHERI_PERM_GLOBAL);

	/* Store local cap through cap without store-local permission */
	targetp = cheri_perms_and(targetp, ~CHERI_PERM_STORE_LOCAL_CAP);
	/* This should fault */
	*targetp = cap;

#ifdef __riscv_zcherilevels
        CHERIBSDTEST_VERIFY(cheri_tag_get(*targetp) == 0);
        cheribsdtest_success();
#else
	cheribsdtest_failure_errx(
	    "No fault after storing local cap via non-store-local cap");
#endif
}
