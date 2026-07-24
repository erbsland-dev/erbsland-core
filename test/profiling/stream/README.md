# Concurrent file-stream profiler

This executable links the common profiling framework and uses its shared application lifecycle and command-line
model. It retains deterministic file fixtures, validation, stream phase measurements, write back-buffer sizing, and
cached expected code-point counts.

`stream-file-profile` exercises Erbsland Core file streams through deterministic binary and encoded-text workloads.
It is intended both for sampling profilers and for comparative timing runs. The default run uses four independent
caller threads for eight minutes, covering all sequential byte and text methods with UTF-8, UTF-16, and UTF-32 files.

## Typical commands

```shell
cmake --build cmake-build-debug --target stream-file-profile
cmake-build-debug/profiling-apps/stream-file-profile --dry-run
cmake-build-debug/profiling-apps/stream-file-profile --scenario binary --threads 1
cmake-build-debug/profiling-apps/stream-file-profile --mode benchmark --scenario text
cmake-build-debug/profiling-apps/stream-file-profile --write-template stream-profile.elcl
cmake-build-debug/profiling-apps/stream-file-profile --config stream-profile.elcl
```

Use `--list-scenarios` to obtain exact scenario IDs for a focused Instruments, `perf`, or debugger launch. A group
such as `binary` or `text` can also be passed to `--scenario`.

The executable can be launched under a sampling profiler without special integration. For example, start the
focused command from Instruments on macOS, or use `perf record` around the same command on Linux. Keep the normal
four workload threads when investigating process-wide contention; pass `--threads 1` only when isolating serial
stream costs. Larger values are useful for separate scaling studies, but should not be mixed into one comparison.

## Configuration and reproducibility

The exported ELCL template contains every run setting and a commented custom-scenario example. Missing run values
use the built-in defaults. Adding one or more `*[scenario]` entries replaces the curated suite. Scalar scenario axes
can be changed into ELCL value lists; `all` expands directions, file types, chunk modes, localities, and compatible
methods.

Run settings control the mode, built-in suite, total deadline, workload threads, global seed, maximum active corpus,
progress interval, optional workspace root, and cleanup policy. Scenario settings control direction, file type,
method list, file and chunk size ranges, chunk strategy, stream-buffering presets, optional output back-buffer limit,
locality, and relative profile weight. Unknown names, invalid ranges, incompatible method combinations, duplicate
expanded IDs, and thread counts outside 1-256 are rejected. Parser and validation diagnostics retain ELCL source
locations where available.

`--dry-run` expands the complete matrix and prints the deterministic selected file size, number of files per worker,
shared or unique assignment, per-file seed, maximum active corpus size, and effective-configuration MD5 without
creating a workspace. `--write-template` exports the checked-in default document, which is also the source embedded
in the executable.

Each file seed is derived from the global seed, expanded scenario ID, worker, and rotating slot. Adding unrelated
scenarios therefore does not change existing inputs. Binary inputs contain pseudorandom bytes. Text inputs contain a
weighted mix of ASCII, punctuation, multiple BMP ranges, supplementary characters, normal lines, and occasional
long lines.

Every workload thread owns its streams. Hot reads open the same immutable file independently; rotating reads use
deterministically assigned files; writes always use worker-specific destinations. The configured thread count does
not control the library's process-wide dynamic I/O service. Output therefore reports `workload-threads` separately
from `io-workers=process-wide-dynamic`. Sharing a cursor-bearing stream object between workload threads is outside
the cover-all suite.

## Validation, timing, and cache locality

MD5 validation runs outside measured samples. Binary files use raw-byte digests. Text reads additionally validate a
canonical UTF-8 digest of decoded output, and text writes validate the exact encoded output including its BOM.
Measured samples still enforce expected lengths, stream statuses, timeout retries, and graceful close completion.

`profile` mode performs a concurrent validation/warm-up sweep before repeating weighted scenarios to the deadline.
`benchmark` mode collects synchronized concurrent samples and reports distribution statistics. Output is stable,
line-oriented `key=value` text suitable for later processing.

The first record identifies the mode, workload concurrency, process-wide I/O-worker policy, seed, duration,
workspace, and effective configuration digest. Scenario records include encoding/file type, method, locality, file
and chunk sizes, buffering preset and resolved capacities, bytes, operation and timeout counts, aggregate wall time,
worker phase times, and
MiB/s. Benchmark records additionally include minimum, median, mean, p95, and maximum wall times plus the minimum
and maximum worker throughput and its coefficient of variation. A high fairness coefficient or a widening
min/max worker range is a useful signal for lock contention or starvation.

For comparisons, use the same executable, seed, configuration digest, thread count, storage device, and build type.
Prefer several complete benchmark runs and compare medians; investigate p95/max and fairness separately instead of
folding them into one score. Corpus construction, checksum calculation, and report formatting are excluded from
sample wall times. An in-progress sample may finish after the deadline, but no new measured pass starts afterward.
Benchmark mode fails with a clear diagnostic if the deadline is too short to obtain its required five samples per
scenario.

The default suite combines hot paths with a bounded rotating corpus. It intentionally does not claim to provide a
portable cold-cache test: operating systems do not expose one consistent, safe cache-eviction mechanism. Use
`--threads 1` for specialized serial profiling and separate runs for scaling comparisons.

The aggregate text `read-all` scenarios use 256 KiB files in the default suite. This path currently scales much more
steeply than the other bulk operations; the bounded size still produces multi-second concurrent samples while
keeping the complete default run inside its turnaround target. Bulk text reads retain the 8 MiB files, and binary
aggregate operations retain the 64 MiB files. Focused configurations can raise the aggregate text size when
investigating that path alone.

Character-at-a-time writes use 4 KiB files because every Unicode scalar becomes an individual stream request; each
four-thread sample still issues roughly twelve thousand calls. Byte-at-a-time and character-at-a-time reads use
256 KiB files.
Chunked Unicode writes use 1 MiB files because the current implementation turns an 8 MiB four-thread sample into a
roughly 40-second block per pass. These bounded defaults preserve broad profiling coverage; focused configurations
remain the right place for intentionally pathological file sizes.
