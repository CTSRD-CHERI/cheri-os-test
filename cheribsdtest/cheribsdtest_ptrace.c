/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 SRI International
 *
 * This software was developed by SRI International, the University of
 * Cambridge Computer Laboratory (Department of Computer Science and
 * Technology), and Capabilities Limited under Defense Advanced Research
 * Projects Agency (DARPA) Contract No. HR001122C0110 ("ETC").
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

#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __linux__
#include <stddef.h>
#include <stdio.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <asm/ptrace.h>
#include <linux/elf.h>
#ifdef __riscv
#include <asm/elf.h>
#endif
#endif

#include "cheribsdtest.h"

/*
 * Fork a child process and attach to it in the parent.  The child
 * exits, but the parent returns the child pid to the caller after
 * attaching to the stopped child.
  */
static pid_t
fork_child(void)
{
	pid_t fpid, wpid;
	int status;

	fpid = fork();
	CHERIBSDTEST_VERIFY2(fpid != -1, "Could not fork: errno=%d", errno);

	if (fpid == 0) {
		/* child */
		CHERIBSDTEST_VERIFY(ptrace(PT_TRACE_ME, 0, NULL, 0) == 0);
		raise(SIGSTOP);

		exit(0);
	}

	/* parent */
	wpid = waitpid(fpid, &status, 0);
	CHERIBSDTEST_VERIFY(wpid == fpid);
	CHERIBSDTEST_VERIFY(WIFSTOPPED(status));

	return (fpid);
}

/* Continue the stopped child process and wait for it to exit. */
static void
finish_child(pid_t pid)
{
	pid_t wpid;
	int status;

	CHERIBSDTEST_VERIFY(ptrace(PT_CONTINUE, pid, (caddr_t)1, 0) == 0);

	wpid = waitpid(pid, &status, 0);
	CHERIBSDTEST_VERIFY(wpid == pid);
	CHERIBSDTEST_VERIFY(WIFEXITED(status));
}

#ifdef __FreeBSD__

CHERIBSDTEST(ptrace_readcap, "Basic tests of PIOD_READ_CHERI_CAP")
{
	struct ptrace_io_desc piod;
	pid_t pid;
	uintcap_t cap, *pp;
	char capbuf[2][sizeof(uintcap_t) + 1];

	pp = malloc(sizeof(*pp) * 2);
	pp[0] = (uintcap_t)(__cheri_tocap void * __capability)&piod;
	pp[1] = 42;

	CHERIBSDTEST_VERIFY(cheri_tag_get(pp[0]) != 0);
	CHERIBSDTEST_VERIFY(cheri_tag_get(pp[1]) == 0);

	pid = fork_child();

	piod.piod_op = PIOD_READ_CHERI_CAP;
	piod.piod_offs = pp;
	piod.piod_addr = capbuf;
	piod.piod_len = sizeof(capbuf);
	CHERIBSDTEST_VERIFY(ptrace(PT_IO, pid, (caddr_t)&piod, 0) == 0);

	CHERIBSDTEST_VERIFY(piod.piod_len == sizeof(capbuf));
	CHERIBSDTEST_VERIFY2(capbuf[0][0] == 1,
	    "Tag not set in returned buffer");
	memcpy(&cap, &capbuf[0][1], sizeof(cap));
	CHERIBSDTEST_VERIFY2(cheri_is_equal_exact(cheri_tag_clear(pp[0]), cap),
	    "Mismatch in non-tag bits of first capability");
	CHERIBSDTEST_VERIFY2(capbuf[1][0] == 0,
	    "Tag set in returned buffer");
	memcpy(&cap, &capbuf[1][1], sizeof(cap));
	CHERIBSDTEST_VERIFY2(cheri_is_equal_exact(pp[1], cap),
	    "Mismatch in non-tag bits of second capability");

	finish_child(pid);

	cheribsdtest_success();
}

CHERIBSDTEST(ptrace_readtags, "Basic test of PIOD_READ_CHERI_TAGS")
{
	struct ptrace_io_desc piod;
	pid_t pid;
	size_t ppsz = 8 * sizeof(uintcap_t);
	uintcap_t *pp;
	char tagbuf[1];

	pp = aligned_alloc(ppsz, ppsz);
	memset(pp, 0, ppsz);

	pp[0] = (uintcap_t)(__cheri_tocap void * __capability)&piod;
	pp[2] = (uintcap_t)(__cheri_tocap void * __capability)tagbuf;

	CHERIBSDTEST_VERIFY(cheri_tag_get(pp[0]) != 0);
	CHERIBSDTEST_VERIFY(cheri_tag_get(pp[1]) == 0);
	CHERIBSDTEST_VERIFY(cheri_tag_get(pp[2]) != 0);
	CHERIBSDTEST_VERIFY(cheri_tag_get(pp[3]) == 0);
	CHERIBSDTEST_VERIFY(cheri_tag_get(pp[4]) == 0);
	CHERIBSDTEST_VERIFY(cheri_tag_get(pp[5]) == 0);
	CHERIBSDTEST_VERIFY(cheri_tag_get(pp[6]) == 0);
	CHERIBSDTEST_VERIFY(cheri_tag_get(pp[7]) == 0);

	pid = fork_child();

	piod.piod_op = PIOD_READ_CHERI_TAGS;
	piod.piod_offs = pp;
	piod.piod_addr = tagbuf;
	piod.piod_len = sizeof(tagbuf);
	CHERIBSDTEST_VERIFY(ptrace(PT_IO, pid, (caddr_t)&piod, 0) == 0);

	CHERIBSDTEST_VERIFY(piod.piod_len == sizeof(tagbuf));
	CHERIBSDTEST_VERIFY(tagbuf[0] == 0x05);

	finish_child(pid);

	cheribsdtest_success();
}

CHERIBSDTEST(ptrace_readcap_pageend,
    "Use PIOD_READ_CHERI_CAP to fetch capability at the end of a page")
{
	struct ptrace_io_desc piod;
	size_t page_size;
	pid_t pid;
	uintcap_t cap, *pp;
	u_int last_index;
	char capbuf[sizeof(uintcap_t) + 1];

	page_size = getpagesize();
	pp = aligned_alloc(page_size, page_size);
	memset(pp, 0, page_size);
	last_index = (page_size / sizeof(uintcap_t)) - 1;
	pp[last_index] = (uintcap_t)(__cheri_tocap void * __capability)&piod;

	CHERIBSDTEST_VERIFY(cheri_tag_get(pp[last_index]) != 0);

	pid = fork_child();

	piod.piod_op = PIOD_READ_CHERI_CAP;
	piod.piod_offs = &pp[last_index];
	piod.piod_addr = capbuf;
	piod.piod_len = sizeof(capbuf);
	CHERIBSDTEST_VERIFY(ptrace(PT_IO, pid, (caddr_t)&piod, 0) == 0);

	CHERIBSDTEST_VERIFY(piod.piod_len == sizeof(capbuf));
	CHERIBSDTEST_VERIFY2(capbuf[0] == 1,
	    "Tag not set in returned buffer");
	memcpy(&cap, &capbuf[1], sizeof(cap));
	CHERIBSDTEST_VERIFY2(cheri_is_equal_exact(
	    cheri_tag_clear(pp[last_index]), cap),
	    "Mismatch in non-tag bits of first capability");

	finish_child(pid);

	cheribsdtest_success();
}

CHERIBSDTEST(ptrace_writecap, "Basic tests of PIOD_WRITE_CHERI_CAP")
{
	struct capreg capreg;
	struct ptrace_io_desc piod;
	pid_t pid;
	int fd;
	uintcap_t *map, pp[2];
	char capbuf[2][sizeof(uintcap_t) + 1];

	fd = CHERIBSDTEST_CHECK_SYSCALL(shm_open(SHM_ANON, O_RDWR, 0600));
	CHERIBSDTEST_CHECK_SYSCALL(ftruncate(fd, getpagesize()));

	map = CHERIBSDTEST_CHECK_SYSCALL(mmap(NULL, getpagesize(),
	    PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

	pid = fork_child();

	/* Fetch the capability registers of the child. */
	CHERIBSDTEST_CHECK_SYSCALL(ptrace(PT_GETCAPREGS, pid, (caddr_t)&capreg,
	    0));

	/* Write a modified PCC with a small offset. */
	pp[0] = CAPREG_PCC(&capreg) + 16;
	pp[1] = 42;

	capbuf[0][0] = 1;
	memcpy(&capbuf[0][1], &pp[0], sizeof(pp[0]));
	capbuf[1][0] = 0;
	memcpy(&capbuf[1][1], &pp[1], sizeof(pp[1]));

	piod.piod_op = PIOD_WRITE_CHERI_CAP;
	piod.piod_offs = map;
	piod.piod_addr = capbuf;
	piod.piod_len = sizeof(capbuf);
	CHERIBSDTEST_CHECK_SYSCALL(ptrace(PT_IO, pid, (caddr_t)&piod, 0));

	CHERIBSDTEST_VERIFY(piod.piod_len == sizeof(capbuf));

	CHERIBSDTEST_VERIFY2(cheri_tag_get(map[0]) == 1,
	    "Tag not set in first injected capability");
	CHERIBSDTEST_VERIFY2(cheri_is_equal_exact(
	    cheri_tag_clear(map[0]), pp[0]),
	    "Mismatch in non-tag bits of first capability");
	CHERIBSDTEST_VERIFY2(cheri_tag_get(map[1]) == 0,
	    "Tag set in second injected capability");
	CHERIBSDTEST_VERIFY2(cheri_is_equal_exact(map[1], pp[1]),
	    "Mismatch in non-tag bits of second capability");

	finish_child(pid);

	cheribsdtest_success();
}

#elif defined(__linux__)
#if defined(__aarch64__)
CHERIBSDTEST(ptrace_getcapregs, "Tests PTRACE_GETREGSET with NT_ARM_MORELLO")
{
	pid_t pid;
	struct user_morello_state regs;
	struct iovec iov = {.iov_base = &regs, .iov_len = sizeof(regs)};

	pid = fork_child();

	CHERIBSDTEST_CHECK_SYSCALL(ptrace(PTRACE_GETREGSET, pid,
		(void *)(uintptr_t)NT_ARM_MORELLO, &iov));

	uint8_t pcc_tag = (regs.tag_map >> MORELLO_PT_TAG_MAP_REG_BIT(pcc)) & 0x1;
	CHERIBSDTEST_VERIFY2(pcc_tag == 1, "PCC tag must be set");
	uint8_t csp_tag = (regs.tag_map >> MORELLO_PT_TAG_MAP_REG_BIT(csp)) & 0x1;
	CHERIBSDTEST_VERIFY2(csp_tag == 1, "CSP tag must be set");

	finish_child(pid);

	cheribsdtest_success();
}

CHERIBSDTEST(ptrace_setcapregs, "Tests PTRACE_SETREGSET with NT_ARM_MORELLO") {
	pid_t pid;
	struct user_morello_state get_regs, set_regs;
	struct iovec get_iov = {.iov_base = &get_regs, .iov_len = sizeof(get_regs)};
	struct iovec set_iov = {.iov_base = &set_regs, .iov_len = sizeof(set_regs)};
	__uint128_t c0_old_val;
	uint8_t     c0_old_tag;
	__uint128_t c0_new_val;
	int ret;

	memset(&get_regs, 0, sizeof(get_regs));
	memset(&set_regs, 0, sizeof(set_regs));

	pid = fork_child();

	// Save original value of c0, so that we can restore it later
	CHERIBSDTEST_CHECK_SYSCALL(ptrace(PTRACE_GETREGSET, pid,
		(void *)(uintptr_t)NT_ARM_MORELLO, &get_iov));
	c0_old_val = get_regs.cregs[0];
	c0_old_tag = get_regs.tag_map & 0x1;

	// Set reg0 to a valid capability
	memcpy(&set_regs, &get_regs, sizeof(set_regs));
	c0_new_val = set_regs.pcc;
	set_regs.cregs[0] = c0_new_val;
	set_regs.tag_map |= 0x1;
	ret = ptrace(PTRACE_SETREGSET, pid, (void *)(uintptr_t)NT_ARM_MORELLO,
		&set_iov);
	CHERIBSDTEST_VERIFY2(ret != -EPERM, "PTRACE_POKECAP is only allowed if " \
									"the sysctl cheri.ptrace_forge_cap is set");

	// Get register set to check if reg0 was set successfully
	memset(&get_regs, 0, sizeof(get_regs));
	CHERIBSDTEST_CHECK_SYSCALL(ptrace(PTRACE_GETREGSET, pid,
		(void *)(uintptr_t)NT_ARM_MORELLO, &get_iov));

	CHERIBSDTEST_VERIFY2(get_regs.cregs[0] == c0_new_val, "c0 wasn't set");
	CHERIBSDTEST_VERIFY2((get_regs.tag_map & 0x1) == 0x1, "Tag wasn't set");

	// Restore c0
	memcpy(&set_regs, &get_regs, sizeof(set_regs));
	set_regs.cregs[0] = c0_old_val;
	if (c0_old_tag == 0)
		set_regs.tag_map &= ~0x1;
	CHERIBSDTEST_CHECK_SYSCALL(ptrace(PTRACE_SETREGSET, pid,
		(void *)(uintptr_t)NT_ARM_MORELLO, &set_iov));

	finish_child(pid);

	cheribsdtest_success();
}

#elif defined(__riscv)
/*
 * XXXPM: The test cases "ptrace_getcapregs" and "ptrace_setcapregs" are in
 *        principle the same on aarch64 and RISCV. It might make sense to
 *        unify the per-architecture test cases, potentially with #ifdefs
 *        within the test cases, to improve maintainability.
 */
CHERIBSDTEST(ptrace_getcapregs,
	"Tests PTRACE_GETREGSET")
{
	pid_t pid;
	elf_gregset_t regs;
	struct iovec iov = {.iov_base = &regs, .iov_len = sizeof(regs)};

	pid = fork_child();

	CHERIBSDTEST_CHECK_SYSCALL(ptrace(PTRACE_GETREGSET, pid, (void *) NT_PRSTATUS,
		&iov));

	uintptr_t bitmap = regs[33];

	CHERIBSDTEST_VERIFY2((bitmap & 1) == 1, "PCC tag must be set");
	CHERIBSDTEST_VERIFY2(((bitmap >> 1) & 1) == 1, "RA tag must be set");
	CHERIBSDTEST_VERIFY2(((bitmap >> 2) & 1) == 1, "SP tag must be set");
	CHERIBSDTEST_VERIFY2(((bitmap >> 3) & 1) == 1, "GP tag must be set");
	CHERIBSDTEST_VERIFY2(((bitmap >> 4) & 1) == 1, "TP tag must be set");

	finish_child(pid);
	cheribsdtest_success();
}

#ifdef __riscv_zcheripurecap
static bool
is_running_on_qemu()
{
	char buf[1024];
	FILE *f = fopen("/sys/firmware/devicetree/base/model", "r");
	while (fgets(buf, sizeof(buf), f)) {
		if (strstr(buf, "qemu")) {
			fclose(f);
			return true;
		}
	}
	fclose(f);
	return false;
}
#endif

CHERIBSDTEST(ptrace_setcapregs,
	"Tests PTRACE_SETREGSET")
{
	pid_t pid;
	elf_gregset_t get_regs, set_regs;
	struct iovec get_iov = {.iov_base = &get_regs, .iov_len = sizeof(get_regs)};
	struct iovec set_iov = {.iov_base = &set_regs, .iov_len = sizeof(set_regs)};
	__uint128_t c5_old_val;
	uint8_t     c5_old_tag;
	__uint128_t c5_new_val;
	int ret;

#ifdef __riscv_zcheripurecap
	/*
	 * This test case trips an assert Qemu. The issue is know:
	 * https://github.com/CHERI-Alliance/qemu/pull/16
	 */
	if (is_running_on_qemu()) {
		cheribsdtest_failure_errx("This test case wasn't executed " \
			"because it crashes Qemu");
	}
#endif

	memset(&get_regs, 0, sizeof(get_regs));
	memset(&set_regs, 0, sizeof(set_regs));

	pid = fork_child();

	// Save original value of c5, so that we can restore it later
	CHERIBSDTEST_CHECK_SYSCALL(ptrace(PTRACE_GETREGSET, pid, (void *) NT_PRSTATUS,
		&get_iov));
	c5_old_val = get_regs[5];
	c5_old_tag = get_regs[33] & 0x1;

	// Set reg5 to a valid capability
	memcpy(&set_regs, &get_regs, sizeof(set_regs));
	c5_new_val = set_regs[0]; // PCC
	set_regs[5] = c5_new_val;
	set_regs[33] |= (0x1 << 5);
	ret = ptrace(PTRACE_SETREGSET, pid, (void *) NT_PRSTATUS, &set_iov);
	cheribsdtest_success();

	CHERIBSDTEST_VERIFY2(ret != -EPERM, "PTRACE_POKECAP is only allowed if " \
									"the sysctl cheri.ptrace_forge_cap is set");

	// Get register set to check if reg5 was set successfully
	memset(&get_regs, 0, sizeof(get_regs));
	CHERIBSDTEST_CHECK_SYSCALL(ptrace(PTRACE_GETREGSET, pid, (void *) NT_PRSTATUS,
		&get_iov));

	CHERIBSDTEST_VERIFY2(get_regs[5] == c5_new_val, "c5 wasn't set");
	CHERIBSDTEST_VERIFY2(((get_regs[33] >> 5) & 0x1) == 0x1, "Tag wasn't set");

	// Restore c5
	memcpy(&set_regs, &get_regs, sizeof(set_regs));
	set_regs[5] = c5_old_val;
	if (c5_old_tag == 0)
		set_regs[33] &= ~(0x1 << 5);
	CHERIBSDTEST_CHECK_SYSCALL(ptrace(PTRACE_SETREGSET, pid, (void *) NT_PRSTATUS,
		&set_iov));

	finish_child(pid);

	cheribsdtest_success();
}
#endif

CHERIBSDTEST(ptrace_peekcap, "Basic tests of ptrace PTRACE_PEEKCAP")
{
	pid_t pid;
	void *tracee_buf[2];
	struct user_cap read_buf[2];

	memset(tracee_buf, 0, sizeof(tracee_buf));
	memset(read_buf, 0, sizeof(read_buf));

	tracee_buf[0] = NULL;
	tracee_buf[1] = &tracee_buf[0];

	CHERIBSDTEST_VERIFY(cheri_tag_get(tracee_buf[0]) == 0);
	CHERIBSDTEST_VERIFY(cheri_tag_get(tracee_buf[1]) == 1);

	pid = fork_child();

	CHERIBSDTEST_VERIFY(ptrace(PTRACE_PEEKCAP, pid, &tracee_buf[0],
		&read_buf[0]) == 0);
	CHERIBSDTEST_VERIFY(ptrace(PTRACE_PEEKCAP, pid, &tracee_buf[1],
		&read_buf[1]) == 0);

	CHERIBSDTEST_VERIFY2(read_buf[0].tag == 0,
		"Tag set for returned integer");
	CHERIBSDTEST_VERIFY2(read_buf[0].val == (uintcap_t) tracee_buf[0],
		"Mismatch in non-tag bits");
	CHERIBSDTEST_VERIFY2(read_buf[1].tag == 1,
		"Tag not set for returned capability");
	CHERIBSDTEST_VERIFY2(read_buf[1].val == (uintcap_t) tracee_buf[1],
		"Mismatch in non-tag bits");

	finish_child(pid);

	cheribsdtest_success();
}

static bool ptrace_forge_cap_sysctl_exists()
{
	const char *path = "/proc/sys/cheri/ptrace_forge_cap";
	struct stat s;
	int ret;

	ret = stat(path, &s);
	if (ret == -1 && errno == ENOENT)
		return false;
	CHERIBSDTEST_VERIFY2(ret == 0, "stat() failed");
	return true;
}

CHERIBSDTEST(ptrace_pokecap, "Basic tests of ptrace PTRACE_POKECAP")
{
	pid_t pid;
	struct user_cap write_cap, read_cap;
	char **map;
	int ret;

	/* XXXPM: Test this on Morello Linux */
	CHERIBSDTEST_VERIFY2(ptrace_forge_cap_sysctl_exists(),
		"sysctl cheri.ptrace_forge_cap does not exist");

	map = aligned_alloc(16, getpagesize());
	memset(map, 0, getpagesize());

	pid = fork_child();

	// Write a capability to the tracee's 'map' buffer
	memcpy(&write_cap.val, &map, 16);
	write_cap.tag = 1;
	ret = ptrace(PTRACE_POKECAP, pid, (caddr_t) &map[0], &write_cap);
	CHERIBSDTEST_VERIFY2(ret != -EPERM, "PTRACE_POKECAP is only allowed if " \
									"the sysctl cheri.ptrace_forge_cap is set");
	CHERIBSDTEST_VERIFY2(ret == 0, "PTRACE_POKECAP failed");

	// Read the capability
	memset(&read_cap, 0, sizeof(read_cap));
	CHERIBSDTEST_VERIFY(ptrace(PTRACE_PEEKCAP, pid, (caddr_t) &map[0],
		&read_cap) == 0);

	CHERIBSDTEST_VERIFY2(read_cap.val == (uintcap_t) map,
		"Written and read capabilities don't match");
	CHERIBSDTEST_VERIFY2(read_cap.tag == 1,
		"Tag not set for written capability");

	finish_child(pid);

	free(map);

	cheribsdtest_success();
}

#endif /* defined(__linux__) */
