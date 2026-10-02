# Constant-time experiments

This separate Erbsland Core `Application` measures two randomly interleaved input populations using Core monotonic time. It is intended for individual investigations after algorithm changes. It is disabled by default, excluded from the default build, and never registered with CTest.

## Build and run

```sh
cmake -S . -B cmake-build-constant-time-release -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DERBSLAND_CORE_ENABLE_TESTS=OFF \
    -DERBSLAND_CORE_ENABLE_DEMOS=OFF \
    -DERBSLAND_CORE_ENABLE_CONSTANT_TIME=ON
cmake --build cmake-build-constant-time-release --target \
    erbsland-core-constant-time erbsland-constant-time-unittest
cmake-build-constant-time-release/test/constant-time/test/unittest/erbsland-constant-time-unittest +tag:FullRun
cmake-build-constant-time-release/test/constant-time/app/erbsland-core-constant-time --list-tests
cmake-build-constant-time-release/test/constant-time/app/erbsland-core-constant-time --self-test
cmake-build-constant-time-release/test/constant-time/app/erbsland-core-constant-time \
    --test aes/128/encrypt --backend both --seed 20261002
```

Use `--all` to run every scenario sequentially, or repeat `--test <exact-id>` to select several. Duplicate selections are removed and execution follows registry order. `--all` and `--test` are mutually exclusive. No selection displays help.

The default budget is 60 seconds **per scenario and backend**, including fixture setup, validation, calibration, and pilot sampling. `--duration-seconds <positive-integer>` overrides it. `--max-samples <positive-integer>` additionally limits post-pilot measurements. `--seed <nonnegative-integer>` reproduces the measurement fixture sequence and class ordering independently of timer-dependent pilot batch sizes; timing itself is not reproducible. A generated seed is printed when none is supplied.

`--backend portable|auto|both` defaults to `both`. The report identifies the concrete selected implementation. Automatic fallback is reported without repeating an identical portable experiment. Some short ChaCha20 paths remain portable even when a vector backend is available. Combined algorithms identify their individual component implementations.

Timing experiments require an optimized build. Debug supports listing, deterministic harness unit tests, and the deterministic part of `--self-test`; its unavailable real-time control returns 3. In an optimized build, `--self-test` succeeds only if its deliberately leaky control is detected.

## Measurement and interpretation

The analysis follows the [dudect algorithm](https://github.com/oreparaz/dudect/blob/master/src/dudect.h): online Welford moments, an uncropped Welch test, 100 exponentially spaced percentile crops established by a discarded pilot, and a centered-square second-order test. An individual channel needs at least 10,000 samples in each population before its statistic can trigger the moderate leakage threshold `|t| > 10`. A completed no-leakage result also requires enough second-order samples. Small sample limits therefore deliberately return insufficient evidence.

Calibration chooses one repetition count using both populations. Samples target at least 10 microseconds and 100 times the observed timer floor. Fixture preparation, expected-result validation, random labels, and harness dispatch stay outside the repetition loop. An opaque checksum sink retains every repeated operation. Batches contain at most 4,096 fixtures and use conservative per-fixture estimates within a 64 MiB budget; expensive preparations reduce batch size further. Progress appears only between batches on an interactive terminal. Redirected output contains plain records.

Results include actual backend, input classes, seed, repetitions, accepted population counts, elapsed time, maximum eligible statistic and channel, and the population counts of that channel. Build/compiler/platform information precedes each measured run. Zero or unusable measurements, degenerate variance, failed validation, and insufficient populations have explicit outcomes. Interruption is cooperative between preparations and samples; an individual algorithm call cannot be interrupted.

Exit codes are 0 for adequate completed runs without detected leakage, 1 for leakage, 2 for configuration/runtime errors, 3 for insufficient evidence or unavailable requested measurements, and 130 for interruption. Across experiments, errors take precedence over leakage, then insufficient evidence. Interruption terminates the sequence.

A clean result is evidence within this budget on this build and platform, not a proof of constant-time execution. A threshold crossing needs a reproducible, focused investigation of the primitive. This tool never changes an algorithm to obtain a clean result. Run measurements sequentially on an otherwise quiet system; guest/emulated platform results should be recorded separately.

## Coverage and extension

The registry initially contains 100 scenarios covering byte containers, AES key expansion/encryption/decryption, GHASH, ChaCha20, Poly1305, authenticated encryption, fixed-output hashes, HMAC, HKDF, password-verifier comparison/protection, TLS Finished checks, equivalent-outcome tag and CBC-padding rejection, X25519 and field arithmetic, Ed25519, ECDSA P-256/P-384, and pinned-key RSA arithmetic/PSS signing. Public-data parsing, signature verification, key generation, and whole memory-hard password derivation are outside measurement. Verification and parsing may be used to validate fixtures before timing.

`app/src` contains the Core application. The local static library in `lib/src` owns the registry, runner, statistics, results, and fixtures. Each algorithm family has its own directory under `lib/src/cases`; each case, fixture, and operation enum has a separate unit. Cases provide metadata and construct fresh backend variants. `PreparedCase<FixtureType>` owns validated fixtures, while each concrete fixture dispatches its operation before the repetition loop. `Result` formats its own plain report, and `RunResults` aggregates exit codes.

Add a family by implementing a case and fixture, adding the explicit named registration in the appropriate `Registry_*.cpp` unit, and listing its units in the local CMake file. Provide stable IDs, equivalent public shapes and outcomes, independently owned fixture data, a conservative memory estimate, and validation outside measurement. Keep sensitive preparation and dispatch outside the repeated operation. Use prepared randomness for signing. The pinned RSA PEM is staged from the existing ordinary-test fixture and embedded using Core resources; the executable does not read a PEM file at runtime or depend on its working directory.
