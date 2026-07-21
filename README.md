# Portable CHERI OS API test suite
This repository provides a portable CHERI test suite for POSIX-based systems such as Linux and CheriBSD. It is heavily based on the [cheribsdtest](https://github.com/CTSRD-CHERI/cheribsd/tree/main/bin/cheribsdtest) suite.

Note, this is a preview and CheriBSD support is currently a work in progress. On CheriBSD, please use the bundled `cheribsdtest` instead. We are currently using the CheriBSD makefiles (see the `mk` directory). This is a short-term interim solution and will be replaced by a cmake-based build system in the near future.

## Building
This test suite can be built with [cheribuild](https://github.com/CTSRD-CHERI/cheribuild). The corresponding `cheribuild` targets are called `cheri-os-test-linux-riscv64-purecap` for RISC-V (RVY) and `cheri-os-test-linux-morello-purecap` for Morello.

The `cheribuild` targets for this test suite are currently available in the `cheriostest` branch of `cheribuild` and will be merged into `main` soon.

## Usage
The test suite binaries are installed in `/opt/cheri-os-test/` by `cheribuild`. `cheriostest-purecap [options..] -a` will run over 200 tests followed by a test report.

### Run select test cases
`cheriostest-purecap -l` lists all available test cases.  
`cheriostest-purecap [options...] <tests> [...]` or `cheriostest-purecap [options...] -g <glob> [...]` run only matching tests.

### Run additional tests
`cheriostest-purecap-mt` includes mutlithreaded test cases.  
`cheriostest-purecap-dynamic` and `cheriostest-purecap-dynamic-mt` include tests that require dynamically linked binaries.
