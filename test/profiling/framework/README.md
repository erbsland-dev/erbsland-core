# Erbsland profiling framework

The framework supplies the application shell, declarative configuration model, scenario registry, deterministic
seeding, synchronized workload runner, statistics, and stable line-oriented reporting for profiling tools.

Every named C++ type has its own matching header and, when required, implementation file. Forward declarations and
incomplete-type aliases belong in the matching `_fwd.hpp` unit. Tool code follows the same layout below its
`app::<domain>` namespace and links `erbsland::profiling`.

A new tool derives its application from `ProfilingApplication`, fills a `ProfilingDefinition` with functionality,
axis, parameter, suite, and metric definitions, and implements `ScenarioWorkload` and `WorkerWorkload` classes for
the measured operations. Checked-in default ELCL files use aligned human-readable names and omit `@features`.

Use benchmark mode for calibrated repeated measurements and profile mode for weighted repetition until the configured
deadline. Fixture preparation and result validation belong in `ScenarioWorkload`; timed mutable state belongs in one
independent `WorkerWorkload` per workload thread.

## Minimal profiler layout

Create `Application_fwd.hpp`, `Application.hpp`, and `Application.cpp`; a matching three-file unit for the shared
scenario workload; and another for its independent worker workload. The full header always includes its matching
`_fwd.hpp`. Put process entry-point functions in a named module and keep only the global `main()` adapter in
`main.cpp`. Do not add declarations to unrelated headers or use anonymous namespaces.

In `configureProfiling()`, set the application metadata and embedded default document, register every functionality,
axis, typed parameter, suite, metric, compatibility predicate, and workload factory, then implement
`executeProfiling()` as `return runRegisteredProfiling();`. A scenario workload prepares immutable shared fixtures,
creates independent workers, and validates aggregate samples. A worker returns operations, definition-ordered metric
counters, an optimizer sink, and an optional digest.

Use `erbsland_add_profiling_executable()` in the tool's `CMakeLists.txt`. Pass the target, source list, default ELCL
file, `DefaultConfiguration.hpp.in`, and its substitution-variable name. The helper embeds the configuration, links
Core and the static profiling framework, enables the project compiler settings, and selects the common output
directory.

The default document starts with `@version`, contains a decorated `[ Run ]` section, and omits `@features`. Add a
smoke document with one inexpensive scenario, invalid/profile/deadline/worker-failure documents when applicable,
dry-run determinism and template checks, and focused tests for registration, expansion, validation, seeding,
workload execution, and metrics.

## Common command line and records

Declarative profilers accept `--config`, `--mode`, `--threads`, `--scenario`, automatically generated axis filters,
`--list-scenarios`, `--list-coverage`, `--dry-run`, and `--write-template`. Output consists of ordered line-oriented
`record=run`, `record=scenario`, `record=sample`, `record=benchmark`, `record=coverage`, and `record=summary` records.
Metrics appear in definition order and rate-enabled counters also emit a `-per-second` field.
