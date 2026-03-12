
#ifndef CHERITESTSUITE_CHERIREG_H
#define CHERITESTSUITE_CHERIREG_H

/*
 * Re-define these because RVY cheriintrin.h uses different names.
 * XXX-AM: Ideally we unify on a single naming convention.
 */
#define CHERI_PERM_STORE                CHERI_PERM_WRITE
#define CHERI_PERM_LOAD                 CHERI_PERM_READ
#define CHERI_PERM_GLOBAL               CHERI_PERM_CAPABILITY_LEVEL
#define CHERI_PERM_STORE_LOCAL_CAP      CHERI_PERM_STORE_LEVEL

/*
 * This file will be removed once CHERI Linux's asm/cheri.h contains
 * these definitions.
 */
#define CHERI_PERM_SW_00 (1 << 6)
#define CHERI_PERM_SW_01 (1 << 7)
#define CHERI_PERM_SW_02 (1 << 8)
#define CHERI_PERM_SW_03 (1 << 9)

#define CHERI_PERMS_FIRST_RESERVED_BLOCK (1 << 2 | 1 << 3 | 1 << 4)


#define CHERI_PERMS_SECOND_RESERVED_BLOCK (1 << 10 | 1 << 11  | 1 << 12 | 1 << 13 | 1 << 14 | 1 << 15)


#define CHERI_PERM_SW_VMEM CHERI_PERM_SW_00

#define CHERI_PERMS_SWALL \
    (CHERI_PERM_SW_00 | CHERI_PERM_SW_01 | CHERI_PERM_SW_02 | CHERI_PERM_SW_03)

#define CHERI_CAP_USER_DATA_PERMS	(CHERI_PERM_WRITE | CHERI_PERM_READ | \
    CHERI_PERM_CAP | CHERI_PERM_LOAD_MUTABLE | CHERI_PERMS_FIRST_RESERVED_BLOCK)

#define CHERI_PERMS_USERSPACE_CODE (CHERI_PERM_EXECUTE | CHERI_PERM_READ)

#endif //CHERITESTSUITE_CHERIREG_H