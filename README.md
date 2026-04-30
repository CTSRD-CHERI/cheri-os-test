# Portable CHERI OS API test suite
This repository provides a portable CHERI test suite for POSIX-based systems such as Linux and CheriBSD. It is heavily based on CheriBSD's [cheribsdtest](https://github.com/CTSRD-CHERI/cheribsd/tree/main/bin/cheribsdtest) suite.

## Building
This test suite is intended to be built with [cheribuild](https://github.com/CTSRD-CHERI/cheribuild). The corresponding cheribuild targets are called `cheri-os-tests-linux-riscv64-purecap` for RISC-V (RVY) and `cheri-os-tests-linux-morello-purecap` for Morello.

## Usage
The test suite binaries are in `/opt/cheri-os-tests/`
`cheribsdtest-purecap [options..] -a` will run over 200 tests followed by a test report

### Run select test cases
`cheribsdtest-purecap -l` lists all test cases.
`cheribsdtest-purecap [options...] <tests> [...]` or `cheribsdtest-purecap [options...] -g <glob> [...]` run only matching tests.

### Run additional tests
`cheribsdtest-purecap-mt` includes mutlithreaded test cases.
`cheribsdtest-purecap-dynamic` and `cheribsdtest-purecap-dynamic-mt` include tests that require dynamically linked binaries. These binaries accept the same parameters as `cheribsdtest-purecap`.

## Note
We are currently using the CheriBSD makefiles (see the `mk` directory). This is a short-term interim solution and will be replaced by a cmake-based build system soon.
