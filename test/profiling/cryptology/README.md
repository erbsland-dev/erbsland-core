# Cryptology hash profiler

`cryptology-hash-profile` hashes deterministic in-memory input through the common framework. Its `algorithm` axis
supports SHA-3, SHA-2, SHA-1, and MD5 variants. The `input-size` parameter defaults to 128 MiB in the `snapshot`
suite. Every sample validates its digest and reports byte throughput.

Use repeatable `--algorithm <id>` filters with the common `--config`, `--mode`, `--threads`, `--scenario`,
`--dry-run`, `--list-scenarios`, `--list-coverage`, and `--write-template` options. Framework calibration replaces
the former iteration and data-size command-line model.

## Configuration and records

The `[ Run ]` section accepts the common `Mode`, `Suite`, `Duration`, `Threads`, `Seed`, `Warmup Samples`, `Samples`,
`Minimum Sample Time`, `Memory Limit`, and `Progress Interval` fields. Scenarios register `Functionality: "hash"`,
one or more `Algorithm` axis values, an `Input Size` byte-length parameter, and an optional profile `Weight`.
External scenarios replace the built-in `snapshot` or `smoke` suite.

The algorithm axis contains `sha3-256`, `sha3-384`, `sha3-512`, `sha-256`, `sha-384`, `sha-512`, `sha-1`, and
`md5`. The
`bytes` metric reports aggregate input throughput; each worker also validates its final digest. Output consists of
ordered `record=run`, `record=scenario`, `record=sample`, `record=benchmark`, `record=coverage`,
`record=progress`, and `record=summary` lines.
