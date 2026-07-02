/*-
 * Copyright (c) 2012-2018, 2020-2021 Robert N. M. Watson
 * Copyright (c) 2014 SRI International
 * Copyright (c) 2021 Microsoft Corp.
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

#ifndef _CHERIOSTEST_H_
#define	_CHERIOSTEST_H_

#include <sys/types.h>

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <cheriintrin.h>

#include "cheriostest_compat.h"

#ifdef __FreeBSD__
#include <sys/linker_set.h>
#include <cheri/cherireg.h>

#include "cheriostest_md.h"
#elif __linux__
#include <bsd/sys/cdefs.h>

#include "cheri/cherireg.h"
#include "utils/linker_set.h"
#ifdef __aarch64__
#include "arm64/cheriostest_md.h"
#elif defined(__riscv)
#include "riscv/cheriostest_md.h"
#endif
#else
#error "Unsupported OS"
#endif

/*
 * We define our own macros for these because they are not portable.
 * Some implementations of CONCAT() do not expand the arguments
 * before concatenating them.
 */
#define CHERITEST_STR(x)		#x
#define CHERITEST_CONC1(x, y)	x ## y
#define CHERITEST_CONC(x, y)	CHERITEST_CONC1(x, y)

/*
 * Convert a pointer to a null-derived void * with the same address. This is
 * useful for getting the correct value for ccs_si_addr_expected.
 */
#define	NULL_DERIVED_VOIDP(x) ((void *)(uintptr_t)(ptraddr_t)(x))

extern int verbose;

/*
 * Shared memory interface between tests and the test controller process.
 */
#define	TESTRESULT_STR_LEN	1024
struct cheriostest_child_state {
	/* Fields filled in by the child signal handler. */
	int		ccs_signum;
	int		ccs_si_code;
	int		ccs_si_trapno;
	void		*ccs_si_addr;

	/* Fields filled in by the test itself. */
	int		ccs_testresult;
	char		ccs_testresult_str[TESTRESULT_STR_LEN];
	void		*ccs_si_addr_expected;
	bool		ccs_warn;
};
extern struct cheriostest_child_state *ccsp;

/*
 * If the test runs to completion, it must set ccs_testresult to SUCCESS or
 * FAILURE.  If the latter, it should also fill ccs_testresult_str with a
 * suitable message to display to the user.
 */
#define	TESTRESULT_UNKNOWN	0	/* Default initialisation. */
#define	TESTRESULT_SUCCESS	1	/* Test declares success. */
#define	TESTRESULT_FAILURE	2	/* Test declares failure. */

/*
 * Description structure for each test -- passed to the test in case it needs
 * access to configuration state, such as strings passed to/from stdio.
 */
#define	CT_FLAG_SIGNAL		0x00000001  /* Should fault; checks signum. */
#define	CT_FLAG_SI_TRAPNO	0x00000002  /* Check signal si_trapno. */
#define	CT_FLAG_STDOUT_STRING	0x00000008  /* Check stdout for a string. */
#define	CT_FLAG_STDIN_STRING	0x00000010  /* Provide string on stdin. */
#define	CT_FLAG_STDOUT_IGNORE	0x00000020  /* Standard output produced,
					       but not checkable */
#define CT_FLAG_SLOW		0x00000040  /* Test is expected to take a 
					       long time to run */
#define	CT_FLAG_SI_CODE		0x00000200  /* Check signal si_code. */
#define	CT_FLAG_SIGEXIT		0x00000400  /* Exits with uncaught signal;
					       checks status signum. */
#define	CT_FLAG_SI_ADDR		0x00000800  /* Check signal si_addr. */

/*
 * Macros defined in one or more cheriostest_md.h to indicate the
 * reason for failure or flaky behavior.  Provide defaults here to
 * reduce the size of MD headers.
 */
#ifndef	SI_CODE_STORELOCAL
#define	SI_CODE_STORELOCAL	PROT_CHERI_STORELOCAL
#endif

#ifndef	XFAIL_HYBRID_BOUNDS_GLOBALS
#ifdef __CHERI_PURE_CAPABILITY__
#define	XFAIL_HYBRID_BOUNDS_GLOBALS	NULL
#else
#define	XFAIL_HYBRID_BOUNDS_GLOBALS \
    "Bounds not supported for globals in hybrid ABI"
#endif
#endif

#ifndef	XFAIL_HYBRID_BOUNDS_GLOBALS_EXTERN
#ifdef __CHERI_PURE_CAPABILITY__
#define	XFAIL_HYBRID_BOUNDS_GLOBALS_EXTERN	NULL
#else
#define	XFAIL_HYBRID_BOUNDS_GLOBALS_EXTERN \
    "Bounds not supported for extern globals in hybrid ABI"
#endif
#endif

#ifndef	XFAIL_HYBRID_BOUNDS_GLOBALS_STATIC
#ifdef __CHERI_PURE_CAPABILITY__
#define	XFAIL_HYBRID_BOUNDS_GLOBALS_STATIC	NULL
#else
#define	XFAIL_HYBRID_BOUNDS_GLOBALS_STATIC \
    "Bounds not supported for static globals in hybrid ABI"
#endif
#endif

#ifndef XFAIL_VARARG_BOUNDS
#define	XFAIL_VARARG_BOUNDS	NULL
#endif

#ifndef XFAIL_C18N_SIGALTSTACK
#ifdef CHERIBSD_C18N_TESTS
#define	XFAIL_C18N_SIGALTSTACK \
    "sigaltstack is currently unsupported by library-based compartmentalisation"
#else
#define	XFAIL_C18N_SIGALTSTACK	NULL
#endif
#endif

#ifndef XFAIL_FLAKY_C18N_CONTEXT
#ifdef CHERIBSD_C18N_TESTS
#define	XFAIL_FLAKY_C18N_CONTEXT \
    "setcontext and swapcontext are currently unsupported by library-based compartmentalisation"
#else
#define	XFAIL_FLAKY_C18N_CONTEXT	NULL
#endif
#endif

struct cheri_test {
	const char	*ct_name;
	const char	*ct_desc;
	void		(*ct_func)(void);
	void		(*ct_child_func)(void);
	const char *	(*ct_check_skip)(const struct cheri_test *);
	const char *	(*ct_check_xfail)(const char *);
	unsigned int	 ct_flags;
	int		 ct_signum;
	int		 ct_si_code;
#ifdef __FreeBSD__
	int		 ct_si_trapno;
#endif
	const char	*ct_stdin_string;
	const char	*ct_stdout_string;
	const char	*ct_xfail_reason;
	const char	*ct_flaky_reason;
};

#define	_CHERIOSTEST_DECLARE(func, desc, ...)				\
	static void func(void);						\
	static struct cheri_test CHERITEST_CONC(__cheri_test, __LINE__) = {	\
		.ct_name = #func,					\
		.ct_desc = (desc),					\
		.ct_func = func,					\
		__VA_ARGS__						\
	};								\
	DATA_SET(cheri_tests_set, CHERITEST_CONC(__cheri_test, __LINE__))

#define	CHERIOSTEST(func, desc, ...)					\
	_CHERIOSTEST_DECLARE(func, (desc), __VA_ARGS__);		\
	static void func(void)

/* Enum for different modes of spawning a child process */
enum spawn_child_mode {
	SC_MODE_POSIX_SPAWN,
	SC_MODE_FORK,
	SC_MODE_VFORK,
#ifdef __FreeBSD__
	SC_MODE_RFORK,
#endif
};

/*
 * Useful APIs for tests.  These terminate the process returning either
 * success or failure with a test-defined, human-readable string describing
 * the error.
 */
void	cheriostest_failure_err(const char *msg, ...) __attribute__((__noreturn__))  __printflike(1, 2);
void	cheriostest_failure_errc(int code, const char *msg, ...) __attribute__((__noreturn__))
    __printflike(2, 3);
void	cheriostest_failure_errx(const char *msg, ...) __attribute__((__noreturn__))  __printflike(1, 2);
void	cheriostest_success(void) __attribute__((__noreturn__));
void	cheriostest_success_with_warn(const char *msg) __attribute__((__noreturn__));
void	signal_handler_clear(int sig);
void	cheriostest_set_expected_si_addr(void *addr);

/**
 * Like CHERIOSTEST_VERIFY but instead of printing condition details prints
 * the provided printf-like message @p fmtargs
 */
#define CHERIOSTEST_VERIFY2(cond, fmtargs...)		\
	do { if (!(cond)) { 				\
		cheriostest_failure_errx(fmtargs);	\
	} } while(0)

/** If @p cond is false fail the test and print the failed condition */
#define CHERIOSTEST_VERIFY(cond) \
	CHERIOSTEST_VERIFY2(cond, "%s", "\'" #cond "\' is FALSE!")

#define CHERIOSTEST_CHECK_EQ(type, fmt, a, b, a_str, b_str)	do {	\
		type __a = (a);						\
		type __b = (b);						\
		CHERIOSTEST_VERIFY2(__a == __b, "%s (" fmt ") == %s ("	\
		    fmt ") failed!", a_str, __a, b_str, __b);		\
	} while (0)

#define CHERIOSTEST_CHECK_EQ_BOOL(a, b)	\
	CHERIOSTEST_CHECK_EQ(_Bool, "%d", a, b, CHERITEST_STR(a), CHERITEST_STR(b))
#define CHERIOSTEST_CHECK_EQ_INT(a, b)	\
	CHERIOSTEST_CHECK_EQ(int, "0x%x", a, b, CHERITEST_STR(a), CHERITEST_STR(b))
#define CHERIOSTEST_CHECK_EQ_LONG(a, b)	\
	CHERIOSTEST_CHECK_EQ(long, "0x%lx", a, b, CHERITEST_STR(a), CHERITEST_STR(b))
#define CHERIOSTEST_CHECK_EQ_SIZE(a, b)	\
	CHERIOSTEST_CHECK_EQ(size_t, "0x%zx", a, b, CHERITEST_STR(a), CHERITEST_STR(b))

static inline void
_cheriostest_check_cap_eq(void *__capability a, void *__capability b,
    const char *a_str, const char *b_str)
{
	/* TODO: This should use CExEq instead once RISC-V has it */
#define CHECK_CAP_ATTR(accessor, fmt)						\
	CHERIOSTEST_VERIFY2(accessor(a) == accessor(b),			\
	    CHERITEST_STR(accessor) "(%s) (" fmt ") == " CHERITEST_STR(accessor)	\
	    "(%s) (" fmt ") failed!", a_str, accessor(a), b_str, accessor(b))
	CHECK_CAP_ATTR(cheri_address_get, "0x%lx");
	CHECK_CAP_ATTR(cheri_tag_get, "%d");
	CHECK_CAP_ATTR(cheri_offset_get, "0x%lx");
	CHECK_CAP_ATTR(cheri_length_get, "0x%lx");
	CHECK_CAP_ATTR(cheri_perms_get, "0x%x");
	CHECK_CAP_ATTR(cheri_type_get, "%ld");
	CHECK_CAP_ATTR(cheri_flags_get, "0x%lx");
#undef CHECK_CAP_ATTR
}
#define CHERIOSTEST_CHECK_EQ_CAP(a, b)	\
	_cheriostest_check_cap_eq(a, b, CHERITEST_STR(a), CHERITEST_STR(b))

#ifdef __CHERI_PURE_CAPABILITY__
#define	CHERIOSTEST_CHECK_EQ_PTR(a, b)	\
	CHERIOSTEST_CHECK_EQ_CAP(a, b)
#else
#define	CHERIOSTEST_CHECK_EQ_PTR(a, b)	\
	CHERIOSTEST_CHECK_EQ(void *, "%p", a, b, __STRING(a), __STRING(b))
#endif

static inline void
_cheriostest_check_cap_bounds_precise(void *__capability c,
    size_t expected_len)
{
	size_t len, offset;

	offset = cheri_offset_get(c);
	len = cheri_length_get(c);

	/* Confirm precise lower bound: offset of zero. */
	CHERIOSTEST_VERIFY2(offset == 0,
	    "offset (%jd) not zero: %#lp", offset, c);

	/* Confirm precise upper bound: length of expected size for type. */
	CHERIOSTEST_VERIFY2(len == expected_len,
	    "length (%jd) not expected %jd: %#lp", len, expected_len, c);
}
#define	CHERIOSTEST_CHECK_CAP_BOUNDS_PRECISE(c, expected_len) \
	_cheriostest_check_cap_bounds_precise((c), (expected_len))

/**
 * Like CHERIOSTEST_CHECK_SYSCALL but instead of printing call details prints
 * the provided printf-like message @p fmtargs
 */
#define CHERIOSTEST_CHECK_SYSCALL2(call, fmtargs...) __extension__({	\
		__typeof(call) __result = call;				\
		if (__result == ((__typeof(__result))-1)) {		\
			cheriostest_failure_err(fmtargs);		\
		}							\
		__result;						\
	})
/**
 * If result of @p call is equal to -1 fail the test and print the failed call
 * followed by the string representation of @c errno
 */
#define CHERIOSTEST_CHECK_SYSCALL(call) \
	CHERIOSTEST_CHECK_SYSCALL2(call, "Call \'" #call "\' failed")

static inline void
_cheriostest_check_errno(const char *context, int actual, int expected)
{
	char actual_str[256];
	char expected_str[256];

	if (expected == actual)
		return;
	if (strerror_r(actual, actual_str, sizeof(actual_str)) != 0)
		cheriostest_failure_err("sterror_r(%d)", actual);
	if (strerror_r(expected, expected_str, sizeof(expected_str)) != 0)
		cheriostest_failure_err("sterror_r(%d)", expected);
	cheriostest_failure_errx("%s errno %d (%s) != expected errno %d (%s)",
	    context, actual, actual_str, expected, expected_str);
}

#ifdef __CHERI_PURE_CAPABILITY__
#define	__CHERIOSTEST_PTR_FMT	"%#p"
#else
#define	__CHERIOSTEST_PTR_FMT	"%p"
#endif

/** Check that @p call fails and errno is set to @p expected_errno */
#define CHERIOSTEST_CHECK_CALL_ERROR(call, expected_errno)		\
	do {								\
		errno = 0;						\
		__typeof(call) __ret = call;				\
		int call_errno = errno;					\
		CHERIOSTEST_VERIFY2(__ret == (__typeof(__ret))-1,	\
		    _Generic((__ret),					\
			void *: #call " unexpectedly returned " __CHERIOSTEST_PTR_FMT, \
			default: #call " unexpectedly returned %d"),	\
		    __ret);						\
		_cheriostest_check_errno(#call, call_errno,		\
		    expected_errno);					\
	} while (0)

#define cheri_ptr(ptr, len)    \
	cheri_bounds_set(    \
	    (__cheri_tocap __typeof__((ptr)[0]) *__capability)ptr, len)

#define cheri_ptrperm(ptr, len, perm)	\
	cheri_perms_and(cheri_ptr(ptr, len), perm | CHERI_PERM_GLOBAL)

/*
 * Return whether the two pointers are equal, including capability metadata if
 * in purecap mode.
 */
static inline bool
cheri_ptr_equal_exact(void *x, void *y)
{
#ifdef __CHERI_PURE_CAPABILITY__
	/* For purecap compare the entire capability including metadata */
	return (cheri_is_equal_exact(x, y));
#else
	/* In hybrid mode void * is just an address */
	return (x == y);
#endif
}


/* For libc_memcpy and libc_memset tests and the unaligned copy tests: */
extern void *cheriostest_memcpy(void *dst, const void *src, size_t n);
extern void *cheriostest_memmove(void *dst, const void *src, size_t n);

extern ptraddr_t find_address_space_gap(size_t len, size_t align);

/*
 * Spawn a new copy of cheribsdtest and run the test's associated child
 * function.
 */
extern pid_t cheriostest_spawn_child(enum spawn_child_mode mode);

const char *skip_need_cheri_revoke(const struct cheri_test *ctp);
const char *skip_need_default_cheri_revoke(const struct cheri_test *ctp);

const char *cheriostest_get_helper_path(void);
const char *cheriostest_skip_no_helper(const struct cheri_test *ctp);

#endif /* !_CHERIOSTEST_H_ */
