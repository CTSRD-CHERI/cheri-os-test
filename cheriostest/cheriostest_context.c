/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Jessica Clarke
 *
 * This software was developed by the University of Cambridge Computer
 * Laboratory (Department of Computer Science and Technology) under Innovate
 * UK project 105694, "Digital Security by Design (DSbD) Technology Platform
 * Prototype".
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


#include <sys/mman.h>

#ifdef __FreeBSD__
#include <sys/signal.h>
#endif

#include <ucontext.h>

#include "cheriostest.h"

#if defined(__musl_libc_heuristic__)
/*
 * The CHERI Linux Project is based on Musl libc which doesn't implement the
 * getcontext(), setcontext(), etc system calls. Likely because they were
 * deprecated by POSIX in 2004.
 */
#define	GET_AND_SETCONTEXT_FAILURE_MESSAGE "getcontext() and related " \
	"functions are not supported by musl libc"

#else
#define	HAS_GET_AND_SETCONTEXT 1
#define	SWAPCONTEXT_ARG1	0x53574150

static int swapcontext_arg1;

static void
swapcontext_func(int arg1)
{
	swapcontext_arg1 = arg1;
}

static void
ucontext_mmap_stack(ucontext_t *uctx)
{
	size_t len;
	void *p;

	len = SIGSTKSZ;
	p = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_ANON, -1, 0);
	CHERIOSTEST_VERIFY2(p != MAP_FAILED, "failed to map new stack");
	uctx->uc_stack.ss_sp = p;
	uctx->uc_stack.ss_size = len;
}

#define	SETCONTEXT_ARG1	0x43485249
#define	SETCONTEXT_ARG2	0x534554

static void
setcontext_func(int arg1, int arg2)
{
	CHERIOSTEST_VERIFY(arg1 == SETCONTEXT_ARG1);
	CHERIOSTEST_VERIFY(arg2 == SETCONTEXT_ARG2);
	cheriostest_success();
}
#endif

CHERIOSTEST(setcontext_basic, "Check that setcontext works",
#ifdef HAS_GET_AND_SETCONTEXT
    /*
     * Currently happens to pass for c18n, possibly because makecontext and
     * setcontext calls are done in the same function?
     */
    .ct_flaky_reason = XFAIL_FLAKY_C18N_CONTEXT
#else
    .ct_xfail_reason = "Not supported"
#endif
)
{
#ifdef HAS_GET_AND_SETCONTEXT
	ucontext_t uc;

	CHERIOSTEST_CHECK_SYSCALL(getcontext(&uc));
	ucontext_mmap_stack(&uc);
	uc.uc_link = NULL;
	makecontext(&uc, (void (*)(void))&setcontext_func, 2, SETCONTEXT_ARG1,
		SETCONTEXT_ARG2);
	CHERIOSTEST_CHECK_SYSCALL(setcontext(&uc));
	cheriostest_failure_errx("returned from successful setcontext");
#else
	cheriostest_failure_errx(GET_AND_SETCONTEXT_FAILURE_MESSAGE);
#endif
}

CHERIOSTEST(swapcontext_basic, "Check that swapcontext works",
#ifdef HAS_GET_AND_SETCONTEXT
    .ct_flaky_reason = XFAIL_FLAKY_C18N_CONTEXT
#else
    .ct_xfail_reason = "Not supported"
#endif
)
{
#ifdef HAS_GET_AND_SETCONTEXT
	ucontext_t uc, uc_link;
	int ret;

	CHERIOSTEST_CHECK_SYSCALL(getcontext(&uc));
	ucontext_mmap_stack(&uc);
	uc.uc_link = &uc_link;
	makecontext(&uc, (void (*)(void))&swapcontext_func, 1,
	    SWAPCONTEXT_ARG1);
	ret = CHERIOSTEST_CHECK_SYSCALL(swapcontext(&uc_link, &uc));
	CHERIOSTEST_VERIFY2(ret == 0, "unknown return value from swapcontext");
	CHERIOSTEST_VERIFY(swapcontext_arg1 == SWAPCONTEXT_ARG1);
	cheriostest_success();
#else
	cheriostest_failure_errx(GET_AND_SETCONTEXT_FAILURE_MESSAGE);
#endif
}
