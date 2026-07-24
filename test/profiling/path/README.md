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
