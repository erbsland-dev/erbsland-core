# PathWalker profiler

`path-walker-profile` measures recursive discovery with both `PathWalker` callback forms and compares them with
`std::filesystem::recursive_directory_iterator`. The `entries` metric reports visited entries per second and each
sample verifies that all implementations discover the same number of paths.

By default, the profiler walks the current working directory. Run it from the tree you want to inspect, or write the
configuration template and add scenarios with a `Root` path parameter:

```elcl
--*[ Scenario ]*---------------------------------------------------------------
Name          : "path-walker"
Functionality : "path-callback"
Root          : "/path/to/tree"

--*[ Scenario ]*---------------------------------------------------------------
Name          : "plain-recursive"
Functionality : "std-recursive"
Root          : "/path/to/tree"
```

```shell
cmake --build cmake-build-debug --target path-walker-profile
cmake-build-debug/profiling-apps/path-walker-profile --dry-run
cmake-build-debug/profiling-apps/path-walker-profile --scenario snapshot:path-callback
cmake-build-debug/profiling-apps/path-walker-profile --scenario snapshot:std-recursive
cmake-build-debug/profiling-apps/path-walker-profile --write-template path-walker-profile.elcl
```

For a focused comparison, keep the same root, executable, build type, and sample settings. The profiler intentionally
does not attempt to evict operating-system directory caches; compare medians from several complete benchmark runs.

## Source-tree read profiling

The opt-in `source-tree` suite helps locate current small-file I/O costs in a sampling profiler. It reads the real
C++ sources beneath `demos`, `src`, and `test`, with separate text and binary scenarios for each directory. The
built-in roots use the repository location recorded by CMake, so the executable can run from another working
directory. Reconfigure after moving the checkout, or supply a custom `Root` as shown below.

Every measured operation rediscovers one complete tree with the `PathInfo` walker callback and reads each selected
file inside that callback. Text scenarios call `PathContent::readTextOrThrow()` with tolerant decoding, matching the
read API used by `update-public-headers`. Binary scenarios call `readDataOrThrow()`. Both use the current default
stream settings and release each result before reading the next file. The walker skips symlinks; the subsequent
read uses `SymlinkMode::Follow`, as the header tool does after discovery. This workload measures the walk and read
APIs, without the header tool's parsing or output generation.

Selection includes regular `.hpp`, `.cpp`, and `.tpp` files, including generated sources in the source tree. It
prunes directories named `.git`, `.cache`, `__pycache__`, `CMakeFiles`, `target`, `build`, or `_build`, and names
beginning with `cmake-build-`. It does not invoke Git or require files to be tracked. Missing roots, empty
selections, traversal failures, and read errors fail the run instead of silently reducing the corpus.

Use an optimized build with symbols for sampling. The existing `cmake-build-profile` configuration uses
`RelWithDebInfo`; a new configuration can be created with:

```shell
cmake -S . -B cmake-build-profile -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DERBSLAND_CORE_ENABLE_TESTS=ON
cmake --build cmake-build-profile --target path-walker-profile
cmake-build-profile/profiling-apps/path-walker-profile --suite source-tree --list-scenarios
cmake-build-profile/profiling-apps/path-walker-profile \
  --suite source-tree --scenario source-tree:src:text --mode profile --duration 60s --threads 1
```

Launch that focused command under Instruments or `perf`. Select exactly one scenario per profile capture: the
shared framework currently spends the profile duration on the first selected scenario. Available IDs follow
`source-tree:<demos|src|test>:<text|data>`. Separate `readTextFile` and `readDataFile` methods make the API paths
recognizable in sampling stacks. There are no per-file timers, logs, or checksums in the measured loop.

For a quick timing summary of all six scenarios, use benchmark mode with one pass per scenario:

```shell
cmake-build-profile/profiling-apps/path-walker-profile \
  --suite source-tree --mode benchmark --samples 1 --warmup-samples 0 --duration 300s
```

One operation is one whole-tree pass. Source-tree scenarios cap batches at one operation per worker, so the
framework's minimum sample time does not multiply tree traversals. `--samples` controls repetition in benchmark
mode. The default is one caller thread; `--threads 2` or higher runs independent callers over the same tree to
investigate contention. This setting does not set the process-wide I/O service's worker count. A pass may finish
after the requested duration; cancellation is checked between files and discards an interrupted pass's counters.

Before sampling, an untimed standard-library inventory counts selected files and their sizes without reading
contents. `record=corpus` reports the resolved root, files per pass, input bytes, and `returned-byte-kind`.
Every completed sample validates the file count; binary samples also validate byte totals. Keep the source tree
unchanged during a run. The corpus naturally changes as development proceeds, and there are no historical
performance thresholds or claims of stable cross-revision comparisons.

Sample and benchmark records include `files`, `files-per-second`, `returned-bytes`, and
`returned-bytes-per-second`. Binary returned bytes are file bytes (`file-data`); text returned bytes are decoded
UTF-8 payload bytes (`decoded-utf8`), which can differ from input sizes due to BOM handling. Tolerant UTF-8 reads
preserve malformed bytes; character operations handle them when interpreting the returned string.
`entries` counts reported directories and selected files in source-tree scenarios. Existing traversal scenarios
retain their entry counting and report zero for the read counters. Warm-up and subsequent passes use normal OS
caching; this is not a cold-cache benchmark. Use sampling stacks for attribution inside traversal, stream setup,
reading, decoding, and cleanup.

To profile another source directory, export the default configuration and add a custom scenario:

```elcl
--*[ Scenario ]*---------------------------------------------------------------
Name          : "my-source-text"
Functionality : "walk-read-text"
Root          : "/path/to/source"
```

Use `walk-read-data` for binary reads. Load this file with `--config`; adding custom scenarios replaces the built-in
suite. Relative custom roots are resolved from the process working directory. The default traversal suite and
its configuration are otherwise unchanged.
