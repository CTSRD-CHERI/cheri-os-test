# Portable CHERI OS API test suite
This repository provides a portable CHERI test suite for POSIX-based systems such as Linux and CheriBSD. It is heavily based on the [cheribsdtest](https://github.com/CTSRD-CHERI/cheribsd/tree/main/bin/cheribsdtest) suite.

Note, this is a preview and CheriBSD support is currently a work in progress. On CheriBSD, please use the bundled `cheribsdtest` instead.

## Building
The test suite is built with CMake, cross-compiling for Linux or CheriBSD on RISC-V or Morello. `CMakePresets.json` has a preset for each combination:

```sh
cmake --preset linux-riscv64    # also freebsd-riscv64, linux-morello, freebsd-morello
cmake --build build/linux-riscv64
```

The presets use `cmake/cheri-purecap-toolchain.cmake`, which expects the SDKs and sysroots to be laid out the way [cheribuild](https://github.com/CTSRD-CHERI/cheribuild) installs them by default, under `~/cheri/output`. Point `CHERI_SDK_ROOT` elsewhere if yours is:

```sh
cmake --preset linux-riscv64 -DCHERI_SDK_ROOT=/path/to/output
```

Either way the compiler and sysroot have to be built by `cheribuild` first, along with `libxo` (and `libbsd` for Linux). `cheribuild` can also configure and build the test suite itself, and on CheriBSD run it in a QEMU VM via CTest.

## Usage
The test suite binaries are installed in `/opt/cheri-os-test/` by `cheribuild`. `cheriostest-purecap [options..] -a` will run over 200 tests followed by a test report.

### Run select test cases
`cheriostest-purecap -l` lists all available test cases.  
`cheriostest-purecap [options...] <tests> [...]` or `cheriostest-purecap [options...] -g <glob> [...]` run only matching tests.

### Run additional tests
`cheriostest-purecap-mt` includes mutlithreaded test cases.  
`cheriostest-purecap-dynamic` and `cheriostest-purecap-dynamic-mt` include tests that require dynamically linked binaries.
