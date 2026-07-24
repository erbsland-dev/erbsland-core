# Regular expression API profiler

This executable links the common profiling framework and uses its shared application lifecycle and command-line
model. Its domain filters select use case, input, pattern, and corpus while retaining deterministic corpora, backend
parity, validation, API coverage, and throughput measurements.

`regex-api-profile` measures compilation and the public matching, searching, collection, and replacement APIs. The
default run is a deterministic four-thread snapshot benchmark with string and seekable-file inputs in UTF-8, UTF-16,
and UTF-32. Use an optimized or `RelWithDebInfo` build for comparisons; Debug builds are intended for smoke testing.

## Typical commands

```shell
cmake --build cmake-build-debug --target regex-api-profile
cmake-build-debug/profiling-apps/regex-api-profile --dry-run
cmake-build-debug/profiling-apps/regex-api-profile --list-coverage
cmake-build-debug/profiling-apps/regex-api-profile --use-case find-all --pattern word
cmake-build-debug/profiling-apps/regex-api-profile --input file-utf16 --threads 1
cmake-build-debug/profiling-apps/regex-api-profile --mode profile
cmake-build-debug/profiling-apps/regex-api-profile --write-template regex-profile.elcl
```

## Coverage and measurement

The snapshot includes compiler representation and size boundaries, alternative and capture-group scaling, character
classes, greedy/lazy/possessive quantifiers, zero-width matches, branch-frontier pressure, Unicode and CRLF handling,
and real-world Shakespeare text and HTML searches. `--list-coverage` prints every supported public overload path.

Benchmark mode calibrates cheap operations, runs synchronized worker samples, and reports min/median/mean/p95/max
nanoseconds per operation, operation and match rates, applicable MiB/s, and worker fairness. File fixtures and streams
are prepared before timing and use warm filesystem cache. Profile mode warms every selected scenario and then repeats
weighted work until the configured deadline.

Output is stable line-oriented `key=value` text. Compare runs only when the executable, configuration MD5, seed,
thread count, build type, and machine conditions agree.
