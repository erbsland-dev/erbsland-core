# Compression Interoperability Tests

This directory contains the standalone compression interoperability executable. It verifies the standard raw payloads
produced and consumed by Erbsland Core against independently maintained codecs behind a Rust counterpart. It uses
Erbsland Unit Test for reporting and selection, but it is deliberately not registered with CTest because it builds and
launches an external-language subprocess.

Enable it explicitly and run it manually:

```shell
cmake -S . -B cmake-build-interop -G Ninja -DCMAKE_BUILD_TYPE=Debug \
    -DERBSLAND_CORE_ENABLE_INTEROP_TESTS=ON
cmake --build cmake-build-interop --target erbsland-core-compression-interop
cmake-build-interop/test/interop/compression/erbsland-core-compression-interop
```

On a shared filesystem that cannot host Cargo build artifacts, place the Rust target directory on a guest-local
filesystem:

```shell
cmake -S . -B cmake-build-interop -G Ninja -DCMAKE_BUILD_TYPE=Debug \
    -DERBSLAND_CORE_ENABLE_INTEROP_TESTS=ON \
    -DERBSLAND_CORE_CARGO_TARGET_DIRECTORY=/path/on/the/guest/interop-cargo
```

## Architecture

For each algorithm, the C++ test writes every source and five Core-compressed payloads into an automatically removed
temporary directory. The payloads represent every portable Core compression level. It launches the Rust counterpart
once per algorithm. The counterpart decodes and compares all Core payloads, then writes several independently compressed
variants for each source. Core decodes every reference variant and compares it with the original source.

The counterpart uses the raw block API from LZ4, raw Deflate through flate2/miniz_oxide, the reference bzip2
implementation, liblzma plus lzma-rs, and the reference Zstandard implementation. Direct and transitive versions are
fixed by `Cargo.toml` and `Cargo.lock`; CMake always builds with Cargo's `--locked` option.

The reference variants deliberately exercise different format choices:

- LZ4 default, accelerated, and high-compression blocks.
- Deflate stored, fixed-Huffman, and dynamic/default streams.
- Bzip2 block sizes 1, 5, and 9.
- LZMA-Alone with a known unpacked size, plus liblzma end-marker streams with default, small-dictionary, and alternate
  literal/position properties. Core decodes each one both with and without a configured expected output length.
- Zstandard frames with and without content size, with different window logs, and with a checksum.

The matrix contains every length from 0 through 33, powers-of-two and codec-specific block boundaries with the adjacent
lengths on either side, all byte values, a large repetitive input, and deterministic mixed inputs up to 192 KiB. The
differential corpus uses the fixed seed `0x6d2b79f5`, and its case names record the derived seed and size. Compressed bytes
are not compared directly because conforming encoders may make different valid choices.

`vectors/manifest.tsv` is an additional static compatibility guard. It contains small compressed payloads produced by
the pinned external encoders, together with exact encoder versions, variants, options, sources, and expected payloads.
The C++ test consumes these committed bytes without invoking Rust. The manifest can be reproduced with:

```shell
cmake-build-interop/test/interop/compression/erbsland-core-compression-interop-counterpart print-vectors
```

The suite also verifies that a non-zero Zstandard dictionary identifier is rejected as an unsupported feature instead
of being silently decoded without the required dictionary.

The complete matrix is intentionally substantial and can take several minutes in a debug build, with Bzip2 accounting
for most of the runtime. Individual algorithms can be selected with the unit-test `name:` filter.

Only `CompressionFormat::Raw` is covered here. Complete-archive interoperability, including ZIP payload routing, is
covered by the sibling `zip` interop suite. Core envelopes, malformed input, and resource-policy checks remain in the
regular compression unit tests.
