# ELCL parser profiler

`conf-parser-profile` uses the common profiling framework to parse the deterministic embedded ELCL corpus. The
`snapshot` and `smoke` suites expose the `parse` functionality and report document and byte rates alongside common
timing and fairness fields.

Use `--config <file>`, `--mode profile|benchmark`, `--threads <count>`, `--scenario <id-or-group>`, `--dry-run`,
`--list-scenarios`, `--list-coverage`, or `--write-template <file>`. Output uses the common ordered record model.
Framework calibration replaces the former iteration option.

## Configuration and records

The `[ Run ]` section accepts the common `Mode`, `Suite`, `Duration`, `Threads`, `Seed`, `Warmup Samples`, `Samples`,
`Minimum Sample Time`, `Memory Limit`, and `Progress Interval` fields. The built-in `snapshot` and `smoke` suites
contain one parser scenario over the embedded corpus. An external `*[ Scenario ]*` section replaces the selected
suite and uses `Name`, `Group`, `Functionality`, `Weight`, and the optional test-only `Fail Worker` parameter.

The functionality is `parse`; its counters are `documents` and `bytes`, both with normalized rates. Output consists
of ordered `record=run`, `record=scenario`, `record=sample`, `record=benchmark`, `record=coverage`,
`record=progress`, and `record=summary` lines.
