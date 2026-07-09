# Portable CHERI OS API test suite
This repository provides a portable CHERI test suite for POSIX-based systems such as Linux and CheriBSD. It is heavily based on CheriBSD's [cheribsdtest](https://github.com/CTSRD-CHERI/cheribsd/tree/main/bin/cheribsdtest) suite.

## Building
This test suite is intended to be built with [cheribuild](https://github.com/CTSRD-CHERI/cheribuild). The corresponding cheribuild targets are called `cheri-os-tests-linux-riscv64-purecap` for RISC-V (RVY) and `cheri-os-tests-linux-morello-purecap` for Morello.

The `cheribuild` targets for this test suite are currently on in the `cheriostest` branch and will be merged into `main` in due course.

## Usage
The test suite binaries are in `/opt/cheri-os-tests/`
`cheriostest-purecap [options..] -a` will run over 200 tests followed by a test report

### Run select test cases
`cheriostest-purecap -l` lists all test cases.
`cheriostest-purecap [options...] <tests> [...]` or `cheriostest-purecap [options...] -g <glob> [...]` run only matching tests.

### Run additional tests
`cheriostest-purecap-mt` includes mutlithreaded test cases.
`cheriostest-purecap-dynamic` and `cheriostest-purecap-dynamic-mt` include tests that require dynamically linked binaries. These binaries accept the same parameters as `cheriostest-purecap`.

## Note
We are currently using the CheriBSD makefiles (see the `mk` directory). This is a short-term interim solution and will be replaced by a cmake-based build system soon.
