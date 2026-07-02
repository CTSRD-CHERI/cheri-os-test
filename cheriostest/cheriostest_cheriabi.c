/*-
 * Copyright (c) 2016 SRI International
 * Copyright (c) 2025-2026 Paul Metzger
 * All rights reserved.
 *
 * This software was developed by the CHERI Research Centre (CRC) in the
 * Department of Computer Science and Technology at the University of
 * Cambridge under the EPSRC grant "UKRI3001: CHERI Research Centre".
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

#if !__has_feature(capabilities)
#error "This code requires a CHERI-aware compiler"
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/time.h>

#ifdef __FreeBSD__
#include <sys/signal.h>
#include <sys/sysctl.h>

#include <machine/sysarch.h>
#elif defined(__linux__)
#include "cheri/cheric.h"
#include "cheri/cherireg.h"
#endif

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __linux__
#include <signal.h>
#endif
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "cheriostest.h"

#ifdef __linux__
#include "cheriostest_compat.h"
#endif

#define	MINCORE_PAGES	3

CHERIOSTEST(cheriabi_mincore,
    "Test CheriABI mincore() with various permissions and bounds")
{
	char *pages, *cap;
	size_t page_sz = getpagesize();
	size_t pages_len = page_sz * MINCORE_PAGES;
	char vec[MINCORE_PAGES];

	pages = CHERIOSTEST_CHECK_SYSCALL(mmap(NULL, pages_len,
	    PROT_MAX(PROT_READ | PROT_WRITE | PROT_EXEC) | PROT_NONE,
	    MAP_ANON | MAP_PRIVATE, -1, 0));

	cap = pages;
	CHERIOSTEST_CHECK_SYSCALL2(mincore(cap, pages_len, vec),
	    "whole allocation from mmap");

	/*
	 * mincore(2) requires minimal permissions, the capability just
	 * needs to be a memory capabilty that can do something useful.
	 */

#if !defined(CHERI_PERM_SW_VMEM)
	cheriostest_failure_errx("CHERI_PERM_SW_VMEM is not defined");
#else
	/* No VMEM */
	cap = cheri_perms_and(pages, ~CHERI_PERM_SW_VMEM);
	CHERIOSTEST_CHECK_SYSCALL2(mincore(cap, pages_len, vec),
	    "whole allocation from mmap without VMEM perm");
#endif

#if defined(__aarch64__)
#define EXEC_ONLY CHERI_PERM_EXECUTE | CHERI_PERM_GLOBAL
#define READ_ONLY CHERI_PERM_LOAD | CHERI_PERM_GLOBAL
#define WRITE_ONLY CHERI_PERM_STORE | CHERI_PERM_GLOBAL
#elif defined(__riscv_zcheripurecap)
/*
 * XXXPM: mincore() fails if the first block of reserved permissions bits is
 *        not set; surprisingly, the second block doesn't need to be set.
 *        The RV64Y permission bit field has reserved blocks, as documented
 *        in https://riscv.github.io/riscv-cheri/#CLRPERM (see Figure 6).
 *        The first two blocks default to 1 and the third to 0.
 *        The specification does not state whether it is legal to unset a
 *        reserved bit that defaults to 1.
 */
#define EXEC_ONLY \
	(CHERI_PERM_EXECUTE | CHERITEST_CHERI_PERMS_FIRST_RESERVED_BLOCK | \
	CHERITEST_CHERI_PERMS_SECOND_RESERVED_BLOCK)
#define READ_ONLY \
	(CHERI_PERM_READ | CHERITEST_CHERI_PERMS_FIRST_RESERVED_BLOCK | \
	CHERITEST_CHERI_PERMS_SECOND_RESERVED_BLOCK)
#define WRITE_ONLY \
	(CHERI_PERM_WRITE | CHERITEST_CHERI_PERMS_FIRST_RESERVED_BLOCK | \
	CHERITEST_CHERI_PERMS_SECOND_RESERVED_BLOCK)
#endif
	/* Execute-only */
	cap = cheri_perms_and(pages, EXEC_ONLY);
	CHERIOSTEST_CHECK_SYSCALL2(mincore(cap, pages_len, vec),
	    "whole allocation from mmap with only CHERI_PERM_EXECUTE");

	/* Read-only */
	cap = cheri_perms_and(pages, READ_ONLY);
	CHERIOSTEST_CHECK_SYSCALL2(mincore(cap, pages_len, vec),
	    "whole allocation from mmap with only CHERI_PERM_LOAD");

	/* Write-only */
	cap = cheri_perms_and(pages, WRITE_ONLY);
	CHERIOSTEST_CHECK_SYSCALL2(mincore(cap, pages_len, vec),
	    "whole allocation from mmap with only CHERI_PERM_STORE");
	/*
	 * mincore(2) needs to work even if the page isn't fully covered.
	 * Restrict bounds to cover a single byte of the first and last
	 * pages.
	 */
	cap = cheritest_trunc_page(cheri_bounds_set(pages + page_sz - 1,
	    pages_len - 2 * (CHERITEST_PAGE_SIZE - 1)));

	/* The whole thing */
	CHERIOSTEST_CHECK_SYSCALL2(mincore(cap, pages_len, vec),
	    "whole allocation with reduced bounds");

	/* 1st page */
	CHERIOSTEST_CHECK_SYSCALL2(mincore(cap, page_sz, vec),
	    "first page (last byte inbounds)");
	CHERIOSTEST_CHECK_SYSCALL2(mincore(cap + page_sz, page_sz, vec),
	    "second page (all in bounds)");
	CHERIOSTEST_CHECK_SYSCALL2(mincore(cap + pages_len - page_sz,
	    page_sz, vec), "last page (first byte in bounds)");

#ifdef __FreeBSD__
	/*
	 * FreeBSD (nonportably) allows under-aligned address and length.
	 */
	CHERIOSTEST_CHECK_SYSCALL2(mincore(cheri_offset_set(cap, 0), 1, vec),
	    "last byte of first page");
	CHERIOSTEST_CHECK_SYSCALL2(mincore(cheri_offset_set(cap, 0),
	    cheri_length_get(cap), vec), "whole in-bounds region");
#endif

	cheriostest_success();
}

CHERIOSTEST(cheriabi_mmap_unrepresentable,
    "Test CheriABI mmap() with unrepresentable lengths")
{
	int shift = 0;
	size_t len;
	size_t expected_len;
	void *cap;
	int prot;
	int flags;

	/*
	 * Generate the shortest unrepresentable length, for which rounding
	 * up to CHERITEST_PAGE_SIZE is still unrepresentable.
	 */
	do {
		len = (1 << (CHERITEST_PAGE_SHIFT + shift)) + 1;
		shift++;
	} while (cheritest_round_page(len) ==
	    __builtin_cheri_round_representable_length(cheritest_round_page(len)));

	expected_len = __builtin_cheri_round_representable_length(len);
#ifdef __linux__
	prot = PROT_READ|PROT_WRITE;
	flags = MAP_ANON | MAP_PRIVATE;
#elif __FreeBSD__
	prot = PROT_READ|PROT_WRITE|PROT_EXEC;
	flags = MAP_ANON;
#endif
	if ((cap = mmap(0, len, prot, flags, -1, 0)) == MAP_FAILED)

		cheriostest_failure_errx("mmap() failed to return a pointer "
		    "when given an unrepresentable length (%zu)", len);
	if (cheri_length_get(cap) != expected_len)
		cheriostest_failure_errx("mmap() returned a pointer with "
		    "an unexpected length (%zu vs %zu) when given an "
		    "unrepresentable length (%zu): %#p", cheri_length_get(cap),
		    expected_len, len, cap);

	cheriostest_success();
}

CHERIOSTEST(cheriabi_mmap_fixed,
    "Verify that we can MAP_FIXED over multiple vm map entries")
{
	void *p1, *p2;

	/* Create a large mapping */
	p1 = mmap(0, 0x200000, PROT_READ | PROT_WRITE,
#ifdef __FreeBSD__
	    MAP_PRIVATE | MAP_ANON | MAP_ALIGNED(21), -1, 0);
#elif defined(__linux__)
	    // mmap on Linux doesn't have a MAP_ALIGNED() flag
	    MAP_PRIVATE | MAP_ANON, -1, 0);
#else
#error "Unsupported OS"
#endif
	CHERIOSTEST_VERIFY(p1 != MAP_FAILED);

	/*
	 * Map over part of the mapping.  This (currently) results
	 * in there being two vm map entries, one of length 0x20000
	 * and another of 0x200000 - 0x20000.
	 */
	p2 = mmap(p1, 0x20000, PROT_READ | PROT_WRITE,
	    MAP_PRIVATE | MAP_FIXED | MAP_ANON, -1, 0);
	CHERIOSTEST_VERIFY(p1 == p2);

	/*
	 * Map over a larger part of the origional mapping spanning
	 * two vm map entries.
	 */
	p2 = mmap(p1, 0x40000, PROT_READ | PROT_WRITE,
	    MAP_PRIVATE | MAP_FIXED | MAP_ANON, -1, 0);
	CHERIOSTEST_VERIFY(p1 == p2);

	cheriostest_success();
}

static int
mmap_and_get_perms(int prot)
{
	void *cap;
	int perms;

	cap = CHERIOSTEST_CHECK_SYSCALL(mmap(NULL, CHERITEST_PAGE_SIZE, prot,
		MAP_ANON | MAP_PRIVATE, -1, 0));
	perms = cheri_perms_get(cap);
	CHERIOSTEST_CHECK_SYSCALL(munmap(cap, CHERITEST_PAGE_SIZE));

	return (perms);
}

CHERIOSTEST(cheriabi_mmap_perms,
    "Verify that mmap returns the correct permissions on capabilities")
{
	int perms;

	/* RO and RW with implied cap perms */
	perms = mmap_and_get_perms(PROT_READ);
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD) != 0,
	    "Missing PERM_LOAD on PROT_READ mapping");
#ifdef HAS_CHERI_PERM_LOAD_MUTABLE
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_MUTABLE) != 0,
	    "Missing PERM_LOAD_MUTABLE on PROT_READ mapping");
#endif
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE) == 0,
	    "Found PERM_STORE on PROT_READ mapping");
#ifdef HAS_CHERI_PERM_LOAD_STORE_CAP
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_CAP) != 0,
	    "Missing PERM_LOAD_CAP on PROT_READ mapping");
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE_CAP) == 0,
	    "Found PERM_STORE_CAP on PROT_READ mapping");
#elif defined(HAS_CHERI_PERM_CAP)
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_CAP) != 0,
	    "Missing PERM_CAP on PROT_READ mapping");
#endif
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_EXECUTE) == 0,
	    "Found PERM_EXEC on PROT_READ mapping");

	perms = mmap_and_get_perms(PROT_READ | PROT_WRITE);
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD) != 0,
	    "Missing PERM_LOAD on PROT_RW mapping");
#ifdef HAS_CHERI_PERM_LOAD_MUTABLE
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_MUTABLE) != 0,
	    "Missing PERM_LOAD_MUTABLE on PROT_RW mapping");
#endif
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE) != 0,
	    "Missing PERM_STORE on PROT_RW mapping");
#ifdef HAS_CHERI_PERM_LOAD_STORE_CAP
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_CAP) != 0,
	    "Missing PERM_LOAD_CAP on PROT_RW mapping");
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE_CAP) != 0,
	    "Missing PERM_STORE_CAP on PROT_RW mapping");
#elif defined(HAS_CHERI_PERM_CAP)
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_CAP) != 0,
	    "Missing PERM_CAP on PROT_RW mapping");
#endif
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_EXECUTE) == 0,
	    "Found PERM_EXEC on PROT_RW mapping");

	perms = mmap_and_get_perms(PROT_READ | PROT_WRITE | PROT_EXEC);
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD) != 0,
	    "Missing PERM_LOAD on PROT_RWX mapping");
#ifdef HAS_CHERI_PERM_LOAD_MUTABLE
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_MUTABLE) != 0,
	    "Missing PERM_LOAD_MUTABLE on PROT_RWX mapping");
#endif
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE) != 0,
	    "Missing PERM_STORE on PROT_RWX mapping");
#ifdef HAS_CHERI_PERM_LOAD_STORE_CAP
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_CAP) != 0,
	    "Missing PERM_LOAD_CAP on PROT_RWX mapping");
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE_CAP) != 0,
	    "Missing PERM_STORE_CAP on PROT_RWX mapping");
#elif defined(HAS_CHERI_PERM_CAP)
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_CAP) != 0,
	    "Missing PERM_CAP on PROT_RWX mapping");
#endif
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_EXECUTE) != 0,
	    "Missing PERM_EXEC on PROT_RWX mapping");

/*
 * CHERI Linux does not have PROT_CAP and PROT_NO_CAP currently.
 */
#ifdef PROT_CAP
	/* RO and RW with explicit cap perms */
	perms = mmap_and_get_perms(PROT_READ | PROT_CAP);
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD) != 0,
	    "Missing PERM_LOAD on PROT_READ | PROT_CAP mapping");
#ifdef HAS_CHERI_PERM_LOAD_MUTABLE
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_MUTABLE) != 0,
	    "Missing PERM_LOAD_MUTABLE on PROT_READ | PROT_CAP mapping");
#endif
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE) == 0,
	    "Found PERM_STORE on PROT_READ | PROT_CAP mapping");
#ifdef HAS_CHERI_PERM_LOAD_STORE_CAP
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_CAP) != 0,
	    "Missing PERM_LOAD_CAP on PROT_READ | PROT_CAP mapping");
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE_CAP) == 0,
	    "Found PERM_STORE_CAP on PROT_READ | PROT_CAP mapping");
#elif defined(HAS_CHERI_PERM_CAP)
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_CAP) != 0,
	    "Missing PERM_CAP on PROT_READ | PROT_CAP mapping");
#endif
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_EXECUTE) == 0,
	    "Found PERM_EXEC on PROT_READ | PROT_CAP mapping");

	perms = mmap_and_get_perms(PROT_READ | PROT_WRITE | PROT_CAP);
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD) != 0,
	    "Missing PERM_LOAD on PROT_RW | PROT_CAP mapping");
#ifdef HAS_CHERI_PERM_LOAD_MUTABLE
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_MUTABLE) != 0,
	    "Missing PERM_LOAD_MUTABLE on PROT_RW | PROT_CAP mapping");
#endif
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE) != 0,
	    "Missing PERM_STORE on PROT_RW | PROT_CAP mapping");
#ifdef HAS_CHERI_PERM_LOAD_STORE_CAP
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_CAP) != 0,
	    "Missing PERM_LOAD_CAP on PROT_RW | PROT_CAP mapping");
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE_CAP) != 0,
	    "Missing PERM_STORE_CAP on PROT_RW | PROT_CAP mapping");
#elif defined(HAS_CHERI_PERM_CAP)
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_CAP) != 0,
	    "Missing PERM_CAP on PROT_RW | PROT_CAP mapping");
#endif
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_EXECUTE) == 0,
	    "Found PERM_EXEC on PROT_RW | PROT_CAP mapping");

	perms = mmap_and_get_perms(PROT_READ | PROT_WRITE | PROT_EXEC |
	    PROT_CAP);
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD) != 0,
	    "Missing PERM_LOAD on PROT_RWX | PROT_CAP mapping");
#ifdef HAS_CHERI_PERM_LOAD_MUTABLE
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_MUTABLE) != 0,
	    "Missing PERM_LOAD_MUTABLE on PROT_RWX | PROT_CAP mapping");
#endif
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE) != 0,
	    "Missing PERM_STORE on PROT_RWX | PROT_CAP mapping");
#ifdef HAS_CHERI_PERM_LOAD_STORE_CAP
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_CAP) != 0,
	    "Missing PERM_LOAD_CAP on PROT_RWX | PROT_CAP mapping");
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE_CAP) != 0,
	    "Missing PERM_STORE_CAP on PROT_RWX | PROT_CAP mapping");
#elif defined(HAS_CHERI_PERM_CAP)
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_CAP) != 0,
	    "Missing PERM_CAP on PROT_RWX | PROT_CAP mapping");
#endif
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_EXECUTE) != 0,
	    "Missing PERM_EXEC on PROT_RWX | PROT_CAP mapping");
#endif

	cheriostest_success();
}

/*
 * CHERI Linux does not define PROT_CAP and PROT_NO_CAP currently.
 */
#ifdef PROT_CAP
CHERIOSTEST(cheriabi_mmap_no_cap_perms,
    "Verify that mmap PROT_NO_CAP does not return capability permissions")
{
	int perms;

	perms = mmap_and_get_perms(PROT_READ | PROT_NO_CAP);
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD) != 0,
	    "Missing PERM_LOAD on PROT_READ | PROT_NO_CAP mapping");
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE) == 0,
	    "Found PERM_STORE on PROT_READ | PROT_NO_CAP mapping");
#ifdef HAS_CHERI_PERM_LOAD_MUTABLE
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_MUTABLE) == 0,
	    "Found PERM_LOAD_MUTABLE on PROT_READ | PROT_NO_CAP mapping");
#endif
#ifdef HAS_CHERI_PERM_LOAD_STORE_CAP
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_CAP) == 0,
	    "Found PERM_LOAD_CAP on PROT_READ | PROT_NO_CAP mapping");
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE_CAP) == 0,
	    "Found PERM_STORE_CAP on PROT_READ | PROT_NO_CAP mapping");
#elif defined(HAS_CHERI_PERM_CAP)
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_CAP) == 0,
	    "Found PERM_CAP on PROT_READ | PROT_NO_CAP mapping");
#endif
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_EXECUTE) == 0,
	    "Found PERM_EXEC on PROT_READ | PROT_NO_CAP mapping");

	perms = mmap_and_get_perms(PROT_READ | PROT_WRITE | PROT_NO_CAP);
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD) != 0,
	    "Missing PERM_LOAD on PROT_RW | PROT_NO_CAP mapping");
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE) != 0,
	    "Missing PERM_STORE on PROT_RW | PROT_NO_CAP mapping");
#ifdef HAS_CHERI_PERM_LOAD_MUTABLE
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_MUTABLE) == 0,
	    "Found PERM_LOAD_MUTABLE on PROT_RW | PROT_NO_CAP mapping");
#endif
#ifdef HAS_CHERI_PERM_LOAD_STORE_CAP
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_CAP) == 0,
	    "Found PERM_LOAD_CAP on PROT_RW | PROT_NO_CAP mapping");
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE_CAP) == 0,
	    "Found PERM_STORE_CAP on PROT_RW | PROT_NO_CAP mapping");
#elif defined(HAS_CHERI_PERM_CAP)
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_CAP) == 0,
	    "Found PERM_CAP on PROT_RW | PROT_NO_CAP mapping");
#endif
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_EXECUTE) == 0,
	    "Found PERM_EXEC on PROT_RW | PROT_NO_CAP mapping");

	perms = mmap_and_get_perms(PROT_READ | PROT_WRITE | PROT_EXEC |
	    PROT_NO_CAP);
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD) != 0,
	    "Missing PERM_LOAD on PROT_RWX | PROT_NO_CAP mapping");
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE) != 0,
	    "Missing PERM_STORE on PROT_RWX | PROT_NO_CAP mapping");
#ifdef HAS_CHERI_PERM_LOAD_MUTABLE
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_MUTABLE) == 0,
	    "Found PERM_LOAD_MUTABLE on PROT_RWX | PROT_NO_CAP mapping");
#endif
#ifdef HAS_CHERI_PERM_LOAD_STORE_CAP
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_CAP) == 0,
	    "Found PERM_LOAD_CAP on PROT_RWX | PROT_NO_CAP mapping");
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE_CAP) == 0,
	    "Found PERM_STORE_CAP on PROT_RWX | PROT_NO_CAP mapping");
#elif defined(HAS_CHERI_PERM_CAP)
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_CAP) == 0,
	    "Found PERM_CAP on PROT_RWX | PROT_NO_CAP mapping");
#endif
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_EXECUTE) != 0,
	    "Missing PERM_EXEC on PROT_RWX | PROT_NO_CAP mapping");

	cheriostest_success();
}

CHERIOSTEST(cheriabi_mmap_maxprot_perms,
    "Verify that mmap PROT_MAX are honored for capability permissions")
{
	int perms;

	perms = mmap_and_get_perms(PROT_MAX(PROT_READ | PROT_CAP) |
	    PROT_READ | PROT_NO_CAP);
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD) != 0,
	    "Missing PERM_LOAD on PROT_MAX(PROT_READ | PROT_CAP) mapping");
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE) == 0,
	    "Found PERM_STORE on PROT_MAX(PROT_READ | PROT_CAP) mapping");
#ifdef HAS_CHERI_PERM_LOAD_MUTABLE
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_MUTABLE) != 0,
	    "Missing PERM_LOAD_MUTABLE on PROT_MAX(PROT_READ | PROT_CAP) mapping");
#endif
#ifdef HAS_CHERI_PERM_LOAD_STORE_CAP
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_CAP) != 0,
	    "Missing PERM_LOAD_CAP on PROT_MAX(PROT_READ | PROT_CAP) mapping");
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE_CAP) == 0,
	    "Found PERM_STORE_CAP on PROT_MAX(PROT_READ | PROT_CAP) mapping");
#elif defined(HAS_CHERI_PERM_CAP)
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_CAP) != 0,
	    "Missing PERM_CAP on PROT_MAX(PROT_READ | PROT_CAP) mapping");
#endif
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_EXECUTE) == 0,
	    "Found PERM_EXEC on PROT_MAX(PROT_READ | PROT_CAP) mapping");

	perms = mmap_and_get_perms(PROT_MAX(PROT_READ | PROT_WRITE | PROT_CAP) |
	    PROT_READ | PROT_WRITE | PROT_NO_CAP);
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD) != 0,
	    "Missing PERM_LOAD on PROT_MAX(PROT_RW | PROT_CAP) mapping");
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE) != 0,
	    "Missing PERM_STORE on PROT_MAX(PROT_RW | PROT_CAP) mapping");
#ifdef HAS_CHERI_PERM_LOAD_MUTABLE
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_MUTABLE) != 0,
	    "Missing PERM_LOAD_MUTABLE on PROT_MAX(PROT_RW | PROT_CAP) mapping");
#endif
#ifdef HAS_CHERI_PERM_LOAD_STORE_CAP
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_CAP) != 0,
	    "Missing PERM_LOAD_CAP on PROT_MAX(PROT_RW | PROT_CAP) mapping");
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE_CAP) != 0,
	    "Missing PERM_STORE_CAP on PROT_MAX(PROT_RW | PROT_CAP) mapping");
#elif defined(HAS_CHERI_PERM_CAP)
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_CAP) != 0,
	    "Missing PERM_CAP on PROT_MAX(PROT_RW | PROT_CAP) mapping");
#endif
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_EXECUTE) == 0,
	    "Found PERM_EXEC on PROT_MAX(PROT_RW | PROT_CAP) mapping");

	perms = mmap_and_get_perms(PROT_MAX(PROT_READ | PROT_WRITE | PROT_EXEC |
	    PROT_CAP) | PROT_READ | PROT_WRITE | PROT_EXEC | PROT_NO_CAP);
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD) != 0,
	    "Missing PERM_LOAD on PROT_MAX(PROT_RWX | PROT_CAP) mapping");
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE) != 0,
	    "Missing PERM_STORE on PROT_MAX(PROT_RWX | PROT_CAP) mapping");
#ifdef HAS_CHERI_PERM_LOAD_MUTABLE
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_MUTABLE) != 0,
	    "Missing PERM_LOAD_MUTABLE on PROT_MAX(PROT_RWX | PROT_CAP) mapping");
#endif
#ifdef HAS_CHERI_PERM_LOAD_STORE_CAP
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_LOAD_CAP) != 0,
	    "Missing PERM_LOAD_CAP on PROT_MAX(PROT_RWX | PROT_CAP) mapping");
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_STORE_CAP) != 0,
	    "Missing PERM_STORE_CAP on PROT_MAX(PROT_RWX | PROT_CAP) mapping");
#elif defined(HAS_CHERI_PERM_CAP)
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_CAP) != 0,
	    "Missing PERM_CAP on PROT_MAX(PROT_RWX | PROT_CAP) mapping");
#endif
	CHERIOSTEST_VERIFY2((perms & CHERI_PERM_EXECUTE) != 0,
	    "Missing PERM_EXEC on PROT_MAX(PROT_RWX | PROT_CAP) mapping");

	cheriostest_success();
}
#endif

struct adjacent_mappings {
	char *first;
	char *middle;
	char *last;
	size_t maplen;
};

/*
 * Create three adjacent memory mappings that be used to check that the memory
 * mapping system calls reject out-of-bounds capabilities that have the address
 * of a valid mapping.
 */
static void
create_adjacent_mappings(struct adjacent_mappings *mappings)
{
	void *requested_addr;
	size_t len;

	len = getpagesize() * 2;
	memset(mappings, 0, sizeof(*mappings));
	requested_addr = (void *)(uintcap_t)find_address_space_gap(len * 3, 0);
	mappings->first = CHERIOSTEST_CHECK_SYSCALL(mmap(requested_addr, len,
	    PROT_READ | PROT_WRITE, MAP_ANON | MAP_FIXED | MAP_PRIVATE, -1, 0));
	CHERIOSTEST_VERIFY(cheri_tag_get(mappings->first));
	/* Try to create a mapping immediately following the latest one. */
	requested_addr =
	    (void *)(uintcap_t)(cheri_address_get(mappings->first) + len);
	mappings->middle = CHERIOSTEST_CHECK_SYSCALL2(mmap(requested_addr, len,
	    PROT_READ | PROT_WRITE, MAP_ANON | MAP_FIXED | MAP_PRIVATE, -1, 0),
	    "Failed to create mapping at address %p", requested_addr);
	CHERIOSTEST_CHECK_EQ_LONG((ptraddr_t)mappings->middle,
	    (ptraddr_t)mappings->first + len);
	requested_addr =
	    (void *)(uintcap_t)(cheri_address_get(mappings->middle) + len);
	CHERIOSTEST_VERIFY(cheri_tag_get(mappings->middle));
	mappings->last = CHERIOSTEST_CHECK_SYSCALL2(mmap(requested_addr, len,
	    PROT_READ | PROT_WRITE, MAP_ANON | MAP_FIXED | MAP_PRIVATE, -1, 0),
	    "Failed to create mapping at address %p", requested_addr);
	CHERIOSTEST_CHECK_EQ_LONG((ptraddr_t)mappings->last,
	    (ptraddr_t)mappings->middle + len);
	CHERIOSTEST_VERIFY(cheri_tag_get(mappings->last));
	mappings->maplen = len;
}

static void
free_adjacent_mappings(struct adjacent_mappings *mappings)
{
	CHERIOSTEST_CHECK_SYSCALL(munmap(mappings->first, mappings->maplen));
	CHERIOSTEST_CHECK_SYSCALL(munmap(mappings->middle, mappings->maplen));
	CHERIOSTEST_CHECK_SYSCALL(munmap(mappings->last, mappings->maplen));
}

CHERIOSTEST(cheriabi_munmap_invalid_ptr,
    "Check that munmap() rejects invalid pointer arguments")
{
	struct adjacent_mappings mappings;

	create_adjacent_mappings(&mappings);

#ifdef __FreeBSD__
	const int expected_errno = EPROT;
#elif defined(__linux__)
	const int expected_errno = EINVAL;
#else
#error "Unsupported OS"
#endif

	/* munmap() with an out-of-bounds length should fail. */
	CHERIOSTEST_CHECK_CALL_ERROR(
	    munmap(mappings.middle, mappings.maplen * 2), expected_errno);
	mappings.middle[0] = 'a'; /* Check that it still has PROT_WRITE */
	CHERIOSTEST_CHECK_CALL_ERROR(
	    munmap(mappings.middle, mappings.maplen + 1), expected_errno);
	mappings.middle[0] = 'a'; /* Check that it still has PROT_WRITE */

	/* munmap() with an in-bounds but untagged capability should fail. */
	CHERIOSTEST_CHECK_CALL_ERROR(
	    munmap(cheri_tag_clear(mappings.middle), mappings.maplen), expected_errno);
	mappings.middle[0] = 'a'; /* Check that the mapping is still valid */

	/* munmap() with an out-of-bounds capability should fail. */
	CHERIOSTEST_CHECK_CALL_ERROR(
	    munmap(mappings.middle - mappings.maplen, mappings.maplen), expected_errno);
	mappings.first[0] = 'a'; /* Check that the mapping is still valid */
	CHERIOSTEST_CHECK_CALL_ERROR(
	    munmap(mappings.middle + mappings.maplen, mappings.maplen), expected_errno);
	mappings.last[0] = 'a'; /* Check that the mapping is still valid */

	/* Unmapping the original capabilities should succeed. */
	free_adjacent_mappings(&mappings);
	cheriostest_success();
}

CHERIOSTEST(cheriabi_mprotect_upgrade_prot_cap,
    "Check that upgrading from PROT_NONE includes capability permissions")
{
	void * volatile *p;

	p = CHERIOSTEST_CHECK_SYSCALL(mmap(NULL, CHERITEST_PAGE_SIZE,
	    PROT_NONE | PROT_MAX(PROT_READ | PROT_WRITE),
	    MAP_ANON | MAP_PRIVATE, -1, 0));
	CHERIOSTEST_CHECK_SYSCALL(mprotect(__DEVOLATILE(void *, p), CHERITEST_PAGE_SIZE,
	    PROT_READ | PROT_WRITE));

	/* Attempt to store a capability */
	*p = __DEVOLATILE(void *, p);

	cheriostest_success();
}

CHERIOSTEST(cheriabi_mprotect_restore_prot_cap,
    "Check that downgrading and then upgrading restores capability permissions")
{
	void * volatile *p;

	p = CHERIOSTEST_CHECK_SYSCALL(mmap(NULL, CHERITEST_PAGE_SIZE,
	    PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0));
	CHERIOSTEST_CHECK_SYSCALL(mprotect(__DEVOLATILE(void *, p),
	    CHERITEST_PAGE_SIZE, PROT_NONE));
	CHERIOSTEST_CHECK_SYSCALL(mprotect(__DEVOLATILE(void *, p),
	    CHERITEST_PAGE_SIZE, PROT_READ | PROT_WRITE));

	/* Attempt to store a capability */
	*p = __DEVOLATILE(void *, p);

	cheriostest_success();
}

#ifdef __FreeBSD__
// This tests the behaviour of mmap with non-POSIX and FreeBSD-specific
// flag PROT_MAX().
CHERIOSTEST(cheriabi_mprotect_downgrade_prot_cap,
    "Check that downgrading to PROT_MAX(PROT_READ) includes capability read",
    .ct_flags = CT_FLAG_SIGNAL | CT_FLAG_SI_CODE | CT_FLAG_SI_TRAPNO | CT_FLAG_SI_ADDR,
    .ct_signum = SIGSEGV,
    .ct_si_code = SEGV_ACCERR,
    .ct_si_trapno = TRAPNO_STORE_PF)
{
	void * volatile *p;

	p = CHERIOSTEST_CHECK_SYSCALL(mmap(NULL, CHERITEST_PAGE_SIZE,
	    PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0));
	*p = __DEVOLATILE(void *, p);

	/* Downgrade and attempt to load a capability */
	CHERIOSTEST_CHECK_SYSCALL(mprotect(__DEVOLATILE(void *, p), CHERITEST_PAGE_SIZE,
	    PROT_READ | PROT_MAX(PROT_READ)));
	CHERIOSTEST_VERIFY(cheri_tag_get(*p));

	/* Try a store.  This should fault. */
	cheriostest_set_expected_si_addr(
	    NULL_DERIVED_VOIDP(__DEVOLATILE(void *, p)));
	*p = __DEVOLATILE(void *, p);

	cheriostest_failure_errx("tagged store succeeded after downgrade");
}
#endif

CHERIOSTEST(cheriabi_mprotect_invalid_ptr,
    "Check that mprotect() rejects invalid pointer arguments")
{
	struct adjacent_mappings mappings;

#ifdef __FreeBSD__
	const int expected_errno = EPROT;
#elif defined(__linux__)
	const int expected_errno = EINVAL;
#else
#error "Unsupported OS"
#endif

	create_adjacent_mappings(&mappings);

	/* mprotect() with an out-of-bounds length should fail. */
	CHERIOSTEST_CHECK_CALL_ERROR(
	    mprotect(mappings.middle, mappings.maplen * 2, PROT_NONE), expected_errno);
	mappings.middle[0] = 'a'; /* Check that it still has PROT_WRITE */
	CHERIOSTEST_CHECK_CALL_ERROR(
	    mprotect(mappings.middle, mappings.maplen + 1, PROT_NONE), expected_errno);
	mappings.middle[0] = 'a'; /* Check that it still has PROT_WRITE */

	/* mprotect() with an in-bounds but untagged capability should fail. */
	CHERIOSTEST_CHECK_CALL_ERROR(mprotect(cheri_tag_clear(mappings.middle),
	    mappings.maplen, PROT_NONE), expected_errno);
	mappings.middle[0] = 'a'; /* Check that it still has PROT_WRITE */

	/* mprotect() with an out-of-bounds capability should fail. */
	CHERIOSTEST_CHECK_CALL_ERROR(mprotect(mappings.middle - mappings.maplen,
	    mappings.maplen, PROT_NONE), expected_errno);
	mappings.first[0] = 'a'; /* Check that it still has PROT_WRITE */
	CHERIOSTEST_CHECK_CALL_ERROR(mprotect(mappings.middle + mappings.maplen,
	    mappings.maplen, PROT_NONE), expected_errno);
	mappings.last[0] = 'a'; /* Check that it still has PROT_WRITE */

	/* Sanity check: mprotect() on a valid capability should succeed. */
	CHERIOSTEST_CHECK_SYSCALL(mprotect(mappings.middle, mappings.maplen,
	    PROT_NONE));
	CHERIOSTEST_CHECK_SYSCALL(mprotect(mappings.middle, mappings.maplen,
	    PROT_READ));

	/* Unmapping the original capabilities should succeed. */
	free_adjacent_mappings(&mappings);
	cheriostest_success();
}

#if __FreeBSD__
// Linux does not have a minherit system call
CHERIOSTEST(cheriabi_minherit_invalid_ptr,
    "Check that minherit() rejects invalid pointer arguments")
{
	struct adjacent_mappings mappings;

	create_adjacent_mappings(&mappings);

	/* minherit() with an out-of-bounds length should fail. */
	CHERIOSTEST_CHECK_CALL_ERROR(minherit(mappings.middle,
	    mappings.maplen * 2, INHERIT_NONE), EPROT);
	CHERIOSTEST_CHECK_CALL_ERROR(minherit(mappings.middle,
	    mappings.maplen + 1, INHERIT_NONE), EPROT);

	/* minherit() with an in-bounds but untagged capability should fail. */
	CHERIOSTEST_CHECK_CALL_ERROR(minherit(cheri_tag_clear(mappings.middle),
	    mappings.maplen, INHERIT_NONE), EPROT);

	/* minherit() with an out-of-bounds capability should fail. */
	CHERIOSTEST_CHECK_CALL_ERROR(minherit(mappings.middle - mappings.maplen,
	    mappings.maplen, INHERIT_NONE), EPROT);
	CHERIOSTEST_CHECK_CALL_ERROR(minherit(mappings.middle + mappings.maplen,
	    mappings.maplen, INHERIT_NONE), EPROT);

	/* Sanity check: minherit() on a valid capability should succeed. */
	CHERIOSTEST_CHECK_SYSCALL(minherit(mappings.middle, mappings.maplen,
	    INHERIT_NONE));
	CHERIOSTEST_CHECK_SYSCALL(minherit(mappings.middle, mappings.maplen,
	    INHERIT_SHARE));

	/* Unmapping the original capabilities should succeed. */
	free_adjacent_mappings(&mappings);
	cheriostest_success();
}
#endif

/*
 * Create three adjacent memory mappings that be used to check that the memory
 * mapping system calls reject out-of-bounds capabilities that have the address
 * of a valid mapping.
 */
static void
create_adjacent_mappings_shm(struct adjacent_mappings *mappings)
{
	void *requested_addr;
	size_t len;
	int shmid;

	len = getpagesize() * 2;
	memset(mappings, 0, sizeof(*mappings));
	shmid = CHERIOSTEST_CHECK_SYSCALL(shmget(IPC_PRIVATE, len, 0600));
	requested_addr = (void *)(uintcap_t)find_address_space_gap(len * 3, 0);
	mappings->first = CHERIOSTEST_CHECK_SYSCALL(shmat(shmid,
	    requested_addr, 0));
	CHERIOSTEST_VERIFY(cheri_tag_get(mappings->first));
	/* Try to create a mapping immediately following the latest one. */
	requested_addr =
	    (void *)(uintcap_t)(cheri_address_get(mappings->first) + len);
	mappings->middle = CHERIOSTEST_CHECK_SYSCALL2(
	    shmat(shmid, requested_addr, 0),
	    "Failed to create mapping at address %p", requested_addr);
	CHERIOSTEST_CHECK_EQ_LONG((ptraddr_t)mappings->middle,
	    (ptraddr_t)requested_addr);
	CHERIOSTEST_VERIFY(cheri_tag_get(mappings->middle));
	requested_addr =
	    (void *)(uintcap_t)(cheri_address_get(mappings->middle) + len);
	mappings->last = CHERIOSTEST_CHECK_SYSCALL2(
	    shmat(shmid, requested_addr, 0),
	    "Failed to create mapping at address %p", requested_addr);
	CHERIOSTEST_CHECK_EQ_LONG((ptraddr_t)mappings->last,
	    (ptraddr_t)requested_addr);
	CHERIOSTEST_VERIFY(cheri_tag_get(mappings->last));
	mappings->maplen = len;
}

static void
free_adjacent_mappings_shm(struct adjacent_mappings *mappings)
{
	CHERIOSTEST_CHECK_SYSCALL(shmdt(mappings->first));
	CHERIOSTEST_CHECK_SYSCALL(shmdt(mappings->middle));
	CHERIOSTEST_CHECK_SYSCALL(shmdt(mappings->last));
}

CHERIOSTEST(cheriabi_shmdt_invalid_ptr,
    "Check that shmdt() rejects invalid pointer arguments")
{
	struct adjacent_mappings mappings;

#ifdef __FreeBSD__
	const int expected_errno = EPROT;
#elif defined(__linux__)
	const int expected_errno = EINVAL;
#else
#error "Unsupported OS"
#endif

	create_adjacent_mappings_shm(&mappings);

	/* shmdt() with an in-bounds but untagged capability should fail. */
	CHERIOSTEST_CHECK_CALL_ERROR(
	    shmdt(cheri_tag_clear(mappings.middle)), expected_errno);
	mappings.middle[0] = 'a'; /* Check that the mapping is still valid */

	/* shmdt() with an out-of-bounds capability should fail. */
	CHERIOSTEST_CHECK_CALL_ERROR(
	    shmdt(mappings.middle - mappings.maplen), expected_errno);
	mappings.first[0] = 'a'; /* Check that the mapping is still valid */
	CHERIOSTEST_CHECK_CALL_ERROR(
	    shmdt(mappings.middle + mappings.maplen), expected_errno);
	mappings.last[0] = 'a'; /* Check that the mapping is still valid */

	/* Unmapping the original capabilities should succeed. */
	free_adjacent_mappings_shm(&mappings);
	cheriostest_success();
}
