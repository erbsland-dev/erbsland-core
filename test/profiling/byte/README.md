# Byte types profiler

This executable links the common profiling framework and uses its shared application lifecycle and command-line
model. Its domain filters select byte type, use case, and sensitivity while retaining deterministic fixtures,
validation digests, API coverage, and throughput measurements.

`byte-types-profile` benchmarks the performance-relevant implementation paths of `ByteArray`, `ByteBlock`,
`ByteBlockEditor`, `ByteBuffer`, and `ByteRingBuffer`. It uses deterministic Erbsland Core data and random generation,
while standard C++20 timing, barriers, and worker threads provide the measurement harness.

With no arguments it runs a four-thread snapshot benchmark with paired normal and sensitive storage scenarios. The
embedded default has a five-minute hard deadline, one warm-up, nine measured samples, and a 20 ms calibration target.
Use an optimized or `RelWithDebInfo` build for performance comparisons; Debug builds are useful only for smoke testing.

## Typical commands

```shell
cmake --build cmake-build-debug --target byte-types-profile
cmake-build-debug/profiling-apps/byte-types-profile --dry-run
cmake-build-debug/profiling-apps/byte-types-profile --list-coverage
cmake-build-debug/profiling-apps/byte-types-profile --type byte-buffer --use-case append
cmake-build-debug/profiling-apps/byte-types-profile --sensitive-mode sensitive
cmake-build-debug/profiling-apps/byte-types-profile --mode profile --threads 1
cmake-build-debug/profiling-apps/byte-types-profile --write-template byte-profile.elcl
cmake-build-debug/profiling-apps/byte-types-profile --config byte-profile.elcl
```

`--scenario` accepts an expanded ID or scenario group. `--type`, `--use-case`, and `--scenario` are repeatable.
`--sensitive-mode normal|sensitive|all` filters configured scenarios; ByteArray remains available and is reported as
`not-applicable` because it has no persistent sensitive mode.

## Coverage model

The profiler describes work as use cases rather than individual overloads. `--list-coverage` prints the central
type/use-case/variant registry and the public API families represented by every path. Convenience overloads that
converge on the same implementation are validated but are not assigned redundant timing scenarios.

The snapshot suite covers lifecycle, indexed and integer access, traversal, comparison/search, bulk mutation,
structural edits, ByteArray bit operations, ring transfers, and capacity management. Variants distinguish shallow and
deep copies, unique and shared storage, COW fan-out, external and aliased inputs, edit position and size relation,
pre-reserved versus growing storage, adversarial search, and fixed versus growing wrapped rings.

Sensitive scenarios cover allocation release, truncation, replacement, detachment, reallocation, ring consumption,
clear/reset, and unique/shared secure erasure. The paired results make the erasure overhead visible without mixing it
into ordinary timings.

The compiled ByteArray extents are `0, 1, 4, 8, 15, 16, 17, 20, 31, 32, 33, 48, 63, 64, 65, 128, 255, 256, 257,
4095, 4096, 4097, 65535, 65536`. Other fixed extents are rejected during configuration expansion.

## ELCL configuration

The `[run]` section controls mode, suite (`snapshot` or `smoke`), deadline, threads, seed, warm-ups, sample count,
minimum sample time, aggregate fixture-memory limit, progress interval, and sensitivity selection. Defining any
`*[scenario]` entry replaces the built-in suite.

Scenario entries select `types`, `use_cases`, compatible `variants`, `size_modes`, primary size bounds, size step,
operand bounds, sensitivity modes, and profile weight. Scalars and ELCL lists are accepted. `all` expands compatible
types, use cases, variants, and sensitivity states.

Size modes expand before measurement:

- `fixed` selects the minimum and, when different, maximum.
- `incrementing` uses `size_step` and includes the maximum.
- `exponential` doubles from the minimum and includes the maximum.
- `random` produces a fixed-seed set without depending on unrelated scenarios.
- `boundaries` selects relevant byte, word, cache-line, growth-block, page, and 64-KiB neighbors inside the range.

Unknown fields, unsupported names or combinations, invalid bounds, duplicate expanded IDs, invalid thread counts, and
unsupported ByteArray extents fail with configuration diagnostics.

## Measurement and output

Source data, immutable baselines, and mutable fixture pools are prepared before timing. Small operations are batched
until the configured sample target or memory limit is reached. Every worker owns mutable fixtures; only immutable
ByteBlock read/copy scenarios share storage across threads. Result bytes, indexes, lengths, capacities, and MD5 digests
are consumed after measurement to prevent dead-code elimination and validate basic invariants.

Output is stable line-oriented `key=value` text. Benchmark records contain min/median/mean/p95/max nanoseconds per
operation, operation rate, applicable MiB/s, capacity changes, and worker-rate fairness/CV. Shallow O(1) operations use
operation normalization and intentionally report zero logical-byte throughput. Profile mode validates and warms every
selected path, then repeats weighted scenarios until the deadline.

Compare runs only when executable, configuration MD5, seed, thread count, build type, and machine conditions match.
Prefer medians from several complete runs; use p95/max and fairness to investigate jitter and contention.
