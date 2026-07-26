/*-
* SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 Alexander Richardson
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

/*
 * Builds a minimal synthetic ELF64 file containing a FreeBSD feature control
 * note, runs set-feature-ctl-note against it, and checks the result.
 */

#include <sys/types.h>
#include <sys/wait.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <err.h>
#include <fcntl.h>
#include <unistd.h>

#include "elf_defs.h"

namespace {

char path[] = "/tmp/test-set-feature-ctl-note.XXXXXX";

/*
 * Writes a synthetic ELF64 file, with one PT_NOTE feature control note if
 * with_note is set, or an empty program header table otherwise.
 */
void
write_test_elf(bool with_note)
{
	Elf64_Ehdr ehdr = {
		.e_ident = { 0x7f, 'E', 'L', 'F', ELFCLASS64, ELFDATA2LSB },
		.e_phoff = sizeof(Elf64_Ehdr),
		.e_phentsize = sizeof(Elf64_Phdr),
		.e_phnum = static_cast<uint16_t>(with_note ? 1 : 0),
	};
	Elf64_Phdr phdr = {
		.p_type = PT_NOTE,
		.p_offset = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr),
		.p_filesz = sizeof(Elf64_Nhdr) + 8 + sizeof(uint32_t),
	};
	Elf64_Nhdr nhdr = {
		.n_namesz = 8,
		.n_descsz = sizeof(uint32_t),
		.n_type = NT_FREEBSD_FEATURE_CTL,
	};
	static const char name[8] = "FreeBSD";
	uint32_t features = 0;

	int fd = open(path, O_WRONLY | O_TRUNC, 0600);
	if (fd < 0)
		err(1, "open %s", path);
	if (write(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr))
		err(1, "write %s", path);
	if (with_note &&
	    (write(fd, &phdr, sizeof(phdr)) != sizeof(phdr) ||
	    write(fd, &nhdr, sizeof(nhdr)) != sizeof(nhdr) ||
	    write(fd, name, sizeof(name)) != sizeof(name) ||
	    write(fd, &features, sizeof(features)) != sizeof(features)))
		err(1, "write %s", path);
	close(fd);
}

uint32_t
read_features()
{
	int fd = open(path, O_RDONLY);
	if (fd < 0)
		err(1, "open %s", path);
	uint32_t v;
	if (pread(fd, &v, sizeof(v), sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr) +
	    sizeof(Elf64_Nhdr) + 8) != sizeof(v))
		err(1, "pread %s", path);
	close(fd);
	return (v);
}

/*
 * Runs "<tool> <arg> path", returning its exit status. tool is our own
 * $<TARGET_FILE:...> and arg is always one of our own literal strings below,
 * so building a shell command from them needs is safe.
 */
int
run_tool(const char *tool, const char *arg)
{
	char cmd[4096];

	snprintf(cmd, sizeof(cmd), "'%s' %s '%s'", tool, arg, path);
	int status = system(cmd);
	return (WIFEXITED(status) ? WEXITSTATUS(status) : -1);
}

} // namespace

int
main(int argc, char **argv)
{
	if (argc != 2)
		errx(1, "usage: %s <path-to-set-feature-ctl-note>", argv[0]);
	const char *tool = argv[1];

	int fd = mkstemp(path);
	if (fd < 0)
		err(1, "mkstemp");
	close(fd);

	write_test_elf(true);
	if (run_tool(tool, "+cherirevoke") != 0)
		errx(1, "+cherirevoke exited non-zero");
	if (read_features() != NT_FREEBSD_FCTL_CHERI_REVOKE_ENABLE)
		errx(1, "+cherirevoke did not set the expected bit");

	write_test_elf(true);
	if (run_tool(tool, "+nocherirevoke") != 0)
		errx(1, "+nocherirevoke exited non-zero");
	if (read_features() != NT_FREEBSD_FCTL_CHERI_REVOKE_DISABLE)
		errx(1, "+nocherirevoke did not set the expected bit");

	/*
	 * The two bits are mutually exclusive: setting one must clear the
	 * other, not just OR the new bit in on top of it.
	 */
	write_test_elf(true);
	if (run_tool(tool, "+cherirevoke") != 0)
		errx(1, "+cherirevoke exited non-zero");
	if (run_tool(tool, "+nocherirevoke") != 0)
		errx(1, "+nocherirevoke exited non-zero");
	if (read_features() != NT_FREEBSD_FCTL_CHERI_REVOKE_DISABLE)
		errx(1, "+nocherirevoke did not clear the +cherirevoke bit");

	/* A file with no feature control note should fail cleanly. */
	write_test_elf(false);
	if (run_tool(tool, "+cherirevoke") == 0)
		errx(1, "expected failure for a file with no PT_NOTE segment");

	unlink(path);
	printf("PASS\n");
	return (0);
}
