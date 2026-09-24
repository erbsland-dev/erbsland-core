# ZIP Interoperability Tests

This directory contains the standalone ZIP interoperability executable. It verifies that Erbsland Core reads archives
from several common implementations and that the pinned Rust `zip` implementation reads archives written by Core. The
executable uses Erbsland Unit Test for reporting and selection, but is deliberately not registered with CTest because
the bidirectional test launches an external-language subprocess.

Enable the target explicitly, build it, and run the committed-vector test:

```shell
cmake -S . -B cmake-build-interop -G Ninja -DCMAKE_BUILD_TYPE=Debug \
    -DERBSLAND_CORE_ENABLE_INTEROP_TESTS=ON
cmake --build cmake-build-interop --target erbsland-core-zip-interop
cmake-build-interop/test/interop/zip/erbsland-core-zip-interop
```

The deterministic bidirectional test is tagged `FullRun` and skipped by default:

```shell
cmake-build-interop/test/interop/zip/erbsland-core-zip-interop +tag:FullRun
```

On a shared filesystem that cannot host Cargo build artifacts, place the Rust target directory on a guest-local
filesystem by setting `ERBSLAND_CORE_CARGO_TARGET_DIRECTORY` while configuring CMake.

## Coverage

The committed corpus under `vectors/` contains independently generated archives from:

- CPython `zipfile`, including stored, Deflate, Bzip2, LZMA, and Zstandard entries, streaming data descriptors, and
  forced per-entry ZIP64 local headers.
- Info-ZIP, using stored and Deflate modes with optional extra fields stripped.
- libarchive `bsdtar`, using stored and Deflate modes with UTF-8 paths.
- Rust `zip`, using stored, Deflate, Bzip2, and Zstandard entries, streaming data descriptors, and forced per-entry
  ZIP64 local headers.

`manifest.tsv` records the exact producer versions and archive variants. `entries.tsv` records the expected entry
order, paths, types, compression methods, comments, timestamps, and payload cases. The static test reads only these
committed files, so it does not require the vector-producing tools at runtime.

The `FullRun` test uses the Rust `zip` crate pinned by `Cargo.toml` and `Cargo.lock`. With the fixed seed `0x7a49504d`,
it creates empty inputs, every size from 0 through 33, boundary-adjacent sizes through 64 KiB, and deterministic mixed
inputs up to 192 KiB. Rust verifies classic and archive-level ZIP64 files from Core using all five Core ZIP methods.
Core verifies classic, streaming, and forced per-entry ZIP64 files from Rust using every method that Rust writes.
Compressed bytes are not compared directly because conforming encoders may make different valid choices.

The metadata scope is UTF-8 ZIP names and comments. Legacy CP437 names and Unicode path/comment extra fields are outside
this suite and remain candidates for separate compatibility work.

## Regenerating the Corpus

Regeneration requires Python 3.14 or later, Info-ZIP `zip`, libarchive `bsdtar`, and Cargo. Run:

```shell
python3 test/interop/zip/generate_vectors.py test/interop/zip/vectors
```

The generator fixes entry data and timestamps, normalizes libarchive's volatile access/change times and numeric owner
metadata, removes stale ZIP archives from the destination, validates every result with Python's independent reader, and
rewrites both TSV manifests. Producer versions in the output intentionally reflect the tools used for that regeneration.
