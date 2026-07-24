# String types profiler

This executable links the common profiling framework and uses its shared application lifecycle and command-line
model. Its domain filters select width, value type, use case, content profile, and sensitivity while retaining
deterministic Unicode fixtures, malformed-input coverage, validation, and throughput measurements.

`string-types-profile` benchmarks the performance-relevant implementation paths of `U8String`, `U16String`,
`U32String`, and their editor types. It uses deterministic Unicode and malformed-data generation while standard
C++20 timing, barriers, and worker threads provide the measurement harness.

With no arguments it runs a four-thread snapshot benchmark. The embedded default has a five-minute hard deadline,
one warm-up, nine measured samples, and a 20 ms calibration target. Use an optimized or `RelWithDebInfo` build for
performance comparisons; Debug builds are useful only for smoke testing.

## Typical commands

```shell
cmake --build cmake-build-debug --target string-types-profile
cmake-build-debug/profiling-apps/string-types-profile --dry-run
cmake-build-debug/profiling-apps/string-types-profile --list-coverage
cmake-build-debug/profiling-apps/string-types-profile --width u8 --type string-editor --use-case append
cmake-build-debug/profiling-apps/string-types-profile --content-profile malformed-sparse --use-case traverse
cmake-build-debug/profiling-apps/string-types-profile --sensitive-mode sensitive --width u8
cmake-build-debug/profiling-apps/string-types-profile --mode profile --threads 1
cmake-build-debug/profiling-apps/string-types-profile --write-template string-profile.elcl
cmake-build-debug/profiling-apps/string-types-profile --config string-profile.elcl
```

The width, type, use-case, content-profile, and scenario filters are repeatable. A scenario filter accepts an expanded
ID or a group. Command-line run overrides are applied before expansion, so `--sensitive-mode all` creates paired U8
normal and sensitive scenarios rather than merely filtering an already expanded suite.

## Coverage model

The central registry describes use cases rather than redundant convenience overloads. `--list-coverage` prints the
type/use-case/variant registry and represented API families. Coverage includes lifecycle and conversion, COW storage,
slicing, native and code-point access, traversal, validation, comparison, hashing, searching, editing, truncation,
transformations, escaping, joining, sensitive U8 storage, and bounded COW/edit stress sequences.

Important variants distinguish native and code-point positions, sequential and random access, unique/shared/sliced
storage, reserved and growing capacity, front/middle/back edits, equal/growing/shrinking replacements, aliased input,
unchanged and changed transformations, and hit/miss/repeated-prefix searches. Concurrent mutation of one editor is
intentionally excluded because editors do not promise shared-mutation safety; each worker owns mutable fixtures.

## Content and sizes

Configured sizes are decoded code-point counts. The tool reports decoded code points, native data units, and native
payload bytes so widths remain comparable without hiding storage costs.

The valid profiles are `ascii`, `mixed`, and `supplementary`. The mixed profile has a fixed distribution of ASCII,
Latin/BMP, CJK, and supplementary code points. `malformed-sparse` and `malformed-dense` inject deterministic invalid
native units and are intended for focused tolerant-decoding runs. The default snapshot uses valid profiles only.
For scan-heavy paths it measures mixed text at 64, 4,096, and 65,536 code points, then adds pure ASCII and
supplementary text at the representative size. This preserves scaling and encoding-shape signals inside the
five-minute default budget.

Size modes expand before measurement:

- `fixed` selects the minimum and, when different, maximum.
- `incrementing` uses `size_step` and includes the maximum.
- `exponential` doubles from the minimum and includes the maximum.
- `random` produces a scenario-local fixed-seed set.
- `boundaries` combines logical size neighbors with code-point prefixes around cache-line, page, compact-growth,
  64-KiB, and 1-MiB native-payload thresholds for the selected width and content profile.

## Reproducibility and output

Corpus seeds derive from the global seed, scenario ID, worker, and sample. Adding an unrelated scenario therefore does
not alter an existing workload. Corpora, immutable baselines, and mutable fixture pools are prepared before timing;
the configured memory limit bounds fixtures across all threads. Construction or destruction remains inside timing
when it is the behavior being profiled.

Output is stable line-oriented `key=value` text. Benchmark records contain min/median/mean/p95/max nanoseconds per
operation, operation and code-point rates, native MiB/s, capacity changes, and worker fairness/CV. Corpus lengths and
encoding status, immutable baselines, COW isolation, and worker completion digests are validated outside timing.

Compare runs only when executable, configuration MD5, seed, thread count, build type, and machine conditions match.
Prefer medians from several complete runs; use p95/max and fairness to investigate jitter and contention.
