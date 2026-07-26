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
 * A minimal replacement for the one thing this test suite needs from
 * elfctl(1): setting the CHERI revocation bits in a binary's FreeBSD feature
 * control note. elfctl needs libelf and FreeBSD's <sys/elf_common.h>, which
 * this project's build host may not have, so parse just enough of the ELF
 * file by hand instead.
 */

#include <sys/types.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <array>

#include <err.h>
#include <fcntl.h>
#include <unistd.h>

#include "elf_defs.h"

namespace {

constexpr uint32_t roundup4(uint32_t x) { return (x + 3) & ~3u; }

[[noreturn]] void usage() {
  errx(1, "usage: set-feature-ctl-note +cherirevoke|+nocherirevoke "
          "<file>");
}

template <typename T> T xpread(int fd, off_t off) {
  T buf{};
  if (pread(fd, &buf, sizeof(buf), off) != static_cast<ssize_t>(sizeof(buf)))
    err(1, "pread");
  return (buf);
}

/* Offset of the feature control note's descriptor, or -1 if there is none. */
off_t find_feature_ctl_desc(int fd, const Elf64_Ehdr &ehdr) {
  for (unsigned int i = 0; i < ehdr.e_phnum; i++) {
    auto phdr = xpread<Elf64_Phdr>(
        fd, static_cast<off_t>(ehdr.e_phoff +
                               static_cast<uint64_t>(i) * ehdr.e_phentsize));
    if (phdr.p_type != PT_NOTE)
      continue;

    uint64_t pos = phdr.p_offset;
    uint64_t end = phdr.p_offset + phdr.p_filesz;
    while (pos + sizeof(Elf64_Nhdr) <= end) {
      auto nhdr = xpread<Elf64_Nhdr>(fd, static_cast<off_t>(pos));
      pos += sizeof(nhdr);
      if (nhdr.n_namesz == 8 && nhdr.n_type == NT_FREEBSD_FEATURE_CTL) {
        auto name = xpread<std::array<char, 8>>(fd, static_cast<off_t>(pos));
        if (memcmp(name.data(), "FreeBSD", 8) == 0) {
          if (nhdr.n_descsz < sizeof(uint32_t))
            errx(1, "feature control note descriptor is too small");
          return (static_cast<off_t>(pos + roundup4(nhdr.n_namesz)));
        }
      }
      pos += roundup4(nhdr.n_namesz) + roundup4(nhdr.n_descsz);
    }
  }
  return (-1);
}

} // namespace

int main(int argc, char **argv) {
  if (argc != 3)
    usage();
  uint32_t bit_to_set, bit_to_clear;
  if (strcmp(argv[1], "+cherirevoke") == 0) {
    bit_to_set = NT_FREEBSD_FCTL_CHERI_REVOKE_ENABLE;
    bit_to_clear = NT_FREEBSD_FCTL_CHERI_REVOKE_DISABLE;
  } else if (strcmp(argv[1], "+nocherirevoke") == 0) {
    bit_to_set = NT_FREEBSD_FCTL_CHERI_REVOKE_DISABLE;
    bit_to_clear = NT_FREEBSD_FCTL_CHERI_REVOKE_ENABLE;
  } else
    usage();
  const char *path = argv[2];

  int fd = open(path, O_RDWR);
  if (fd < 0)
    err(1, "open %s", path);

  auto ehdr = xpread<Elf64_Ehdr>(fd, 0);
  if (memcmp(ehdr.e_ident, "\177ELF", 4) != 0)
    errx(1, "%s is not an ELF file", path);
  if (ehdr.e_ident[EI_CLASS] != ELFCLASS64 ||
      ehdr.e_ident[EI_DATA] != ELFDATA2LSB)
    errx(1, "%s is not a little-endian ELF64 file", path);
  if (ehdr.e_phentsize != sizeof(Elf64_Phdr))
    errx(1, "%s has unexpected program header size", path);

  off_t desc_off = find_feature_ctl_desc(fd, ehdr);
  if (desc_off < 0)
    errx(1, "%s has no NT_FREEBSD_FEATURE_CTL note", path);

  auto features = xpread<uint32_t>(fd, desc_off);
  features = (features & ~bit_to_clear) | bit_to_set;
  if (pwrite(fd, &features, sizeof(features), desc_off) !=
      static_cast<ssize_t>(sizeof(features)))
    err(1, "pwrite %s", path);

  close(fd);
  return (0);
}
