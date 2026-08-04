# Fuzzing

Each fuzz target resides in a separate subdirectory. This keeps its source,
build configuration, corpus, and launcher together as more targets are added.

## TLS 1.3 record deprotection

From the repository root, start the bounded TLS record-deprotection fuzzer with:

```shell
test/fuzzing/tls-record/run.sh -max_total_time=60
```

The first input byte selects one of the three supported cipher suites. The rest
is one exact record candidate, capped at the RFC 8446 ciphertext bound. Checked-in
seeds cover valid, malformed, oversized, and authentication-tampered records.

## TLS 1.3 client protocol

From the repository root, start the bounded client state-machine fuzzer with:

```shell
test/fuzzing/tls-client-protocol/run.sh -max_total_time=60
```

Its first byte selects raw pre-handshake transport input, authenticated post-handshake messages, alerts, application
data, or raw established-state transport input. The target establishes a deterministic valid TLS 1.3 core when needed,
feeds bounded variable-size fragments, and exercises parser, state, key-transition, queue, alert, and erasure paths.

## TLS 1.3 server protocol

From the repository root, start the bounded server state-machine fuzzer with:

```shell
test/fuzzing/tls-server-protocol/run.sh -max_total_time=60
```

Its first byte selects raw pre-checkpoint ClientHello transport input, authenticated post-handshake messages, alerts,
application data, or raw established-state transport input. The target embeds an Ed25519 test identity, establishes a
real client/server core pair for authenticated modes, and feeds fragments under the 64-KiB input cap.

## ECDSA signature verification

From the repository root, start the structured ECDSA verifier fuzzer with:

```shell
test/fuzzing/ecdsa-signature/run.sh
```

The Punycode/IDNA target exercises pure encoding and decoding plus strict network IDNA2008 processing:

```shell
test/fuzzing/punycode/run.sh -max_total_time=60
```

The input carries independently bounded SPKI, signature ``AlgorithmIdentifier``,
message, and DER signature fields. Checked-in hexadecimal sources seed valid
compressed and uncompressed P-256 paths; the launcher converts them to the
binary corpus consumed by libFuzzer.

## X.509 DER decoder

From the repository root, start the X.509 DER fuzzer with:

```shell
test/fuzzing/x509-der/run.sh
```

The launcher configures `cmake-build-fuzzing`, builds `x509-der-fuzz`, creates
its corpus in `cmake-build-fuzzing/corpus/x509-der`, and begins fuzzing. It
uses `/opt/local/bin/clang++-mp-22` by default. Override that compiler or the
build directory when needed:

```shell
CXX=/path/to/clang++ test/fuzzing/x509-der/run.sh -max_total_time=60
ERBSLAND_CORE_FUZZ_BUILD_DIR=/tmp/core-fuzz test/fuzzing/x509-der/run.sh -runs=1000
```

The launcher converts the checked-in `x509-der/corpus/example-certificate.pem`
into DER and adds it to the mutable build corpus on every run. This provides a
valid X.509 certificate from the first execution; add further PEM seed
certificates in that directory when they exercise distinct parser paths.

Arguments following the script name are forwarded to libFuzzer. Useful ones
include `-max_total_time=60` for a bounded run and `-runs=1000` for a quick
smoke test. Press Ctrl-C to stop an unbounded run.

To reproduce a crash, pass it directly to the built fuzzer executable:

```shell
cmake-build-fuzzing/test/fuzzing/x509-der/x509-der-fuzz crash-<hash>
```
