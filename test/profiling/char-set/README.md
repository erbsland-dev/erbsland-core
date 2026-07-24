# Character-set profiler

`char-set-profile` benchmarks the complete public `CharSet` API through the common profiling framework. It is intended
to compare representation and normalization changes, especially optimizations for empty sets, single characters, and
single ranges.

The built-in snapshot covers every constructor and assignment path, comparison and predicate, range accessor, set
operation and operator, mutation overload, iteration mode, Unicode transformation, conversion, and factory overload.
Representative cases distinguish empty, inline-sized, adjacent, sparse, overlapping, disjoint, ASCII-category, and
Unicode-category inputs. `--list-coverage` prints the API-to-functionality map.

## Typical commands

```shell
cmake --build cmake-build-release --target char-set-profile
cmake-build-release/profiling-apps/char-set-profile --dry-run
cmake-build-release/profiling-apps/char-set-profile --list-coverage
cmake-build-release/profiling-apps/char-set-profile --scenario snapshot:construct-char:ascii
cmake-build-release/profiling-apps/char-set-profile --scenario snapshot:contains:sparse-last
cmake-build-release/profiling-apps/char-set-profile --scenario snapshot:from-unicode-category:decimal-number
cmake-build-release/profiling-apps/char-set-profile --suite allocations --scenario allocations:construct-char:ascii
cmake-build-release/profiling-apps/char-set-profile --mode profile --threads 1
cmake-build-release/profiling-apps/char-set-profile --write-template char-set-profile.elcl
```

Use an optimized or `RelWithDebInfo` build for comparisons. The checked-in smoke configuration is intentionally small
and is suitable for Debug validation only.

## Allocation metrics

The executable replaces the standard C++ allocation functions. The default `snapshot` suite keeps the counters
disabled for timing runs. The matching `allocations` suite enables thread-local counters only around each worker's
operation loop, recording allocations caused by the measured API and destruction of its result while excluding
scenario and worker fixture preparation. Custom scenarios control this with `Track Allocations`. The reported metrics
are:

- `tracked-operations`: operations represented by the metric totals.
- `characters`: logical source scalar values processed by those operations.
- `allocations`: successful ordinary and aligned allocation calls.
- `allocated-bytes`: requested bytes for those allocations.
- `deallocations`: deallocation calls observed in the same interval.

Divide allocation totals by `tracked-operations` to obtain per-operation counts. Benchmark timing still comes from the
common framework and includes only a small fixed worker-dispatch overhead around the measured loop. Compare results
only when executable, build type, configuration digest, seed, thread count, and machine conditions match.

Custom scenarios select a registered `Functionality` and one of the case names shown by the built-in dry run. The
`Case` text is deliberately explicit so saved configurations identify the exact normalized range shape being tested.
