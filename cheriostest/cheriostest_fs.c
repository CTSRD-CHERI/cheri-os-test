/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 SRI International
 * Copyright (c) 2025 Paul Metzger
 *
 * This software was developed by the CHERI Research Centre (CRC) in the
 * Department of Computer Science and Technology at the University of
 * Cambridge under the EPSRC grant "UKRI3001: CHERI Research Centre".
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

#include <sys/param.h>
#include <sys/mount.h>
#ifdef __linux__
#include "sys/vfs.h"

#include "linux/magic.h"
#endif

#include <err.h>
#include <stdlib.h>
#include <unistd.h>

#include "cheriostest.h"

static const char *
skip_non_tmpfs_tmp(const struct cheri_test *ctp __attribute__((__unused__)))
{
	struct statfs sb;

	if (statfs("/tmp", &sb) != 0) {
		warn("%s: statfs(\"/tmp\")", __func__);
		return ("unable to query statfs for /tmp");
	}

#ifdef __FreeBSD__
	if (strcmp(sb.f_fstypename, "tmpfs") == 0)
		return (NULL);
#elif defined(__linux__)
	if (sb.f_type == TMPFS_MAGIC)
		return (NULL);
#endif
	else
		return ("/tmp is not using tmpfs");
}

static int
create_tempfile(void)
{
	char template[] = "/tmp/cheribsdtest.XXXXXXXX";
	int fd = CHERIOSTEST_CHECK_SYSCALL2(mkstemp(template),
	    "mkstemp %s", template);
	CHERIOSTEST_CHECK_SYSCALL(unlink(template));
	CHERIOSTEST_CHECK_SYSCALL(ftruncate(fd, getpagesize()));
	return (fd);
}

CHERIOSTEST(tmpfs_rw_nocaps,
    "check that read(2) and write(2) of tmpfs files do not return tags",
    .ct_check_skip = skip_non_tmpfs_tmp)
{
	void * __capability c;
	void * __capability d;
	size_t rv;
	int fd;

	fd = create_tempfile();

	/* Just some pointer */
	c = &fd;
	CHERIOSTEST_VERIFY2(cheri_tag_get(c) != 0, "tag set on source");

	rv = CHERIOSTEST_CHECK_SYSCALL(pwrite(fd, &c, sizeof(c), 0));
	CHERIOSTEST_CHECK_EQ_SIZE(rv, sizeof(c));

	rv = CHERIOSTEST_CHECK_SYSCALL(pread(fd, &d, sizeof(d), 0));
	CHERIOSTEST_CHECK_EQ_SIZE(rv, sizeof(d));

	CHERIOSTEST_VERIFY2(cheri_tag_get(d) == 0, "tag read");
	CHERIOSTEST_VERIFY2(cheri_is_equal_exact(cheri_tag_clear(c), d),
	    "untagged value not read");

	CHERIOSTEST_CHECK_SYSCALL(close(fd));
	cheriostest_success();
}
