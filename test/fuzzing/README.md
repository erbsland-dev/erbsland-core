# Fuzzing

Each fuzz target resides in a separate subdirectory with its C++ sources in
`src`, build configuration in `CMakeLists.txt`, checked-in seeds in `corpus`, and
launcher in `run.sh`. The TLS targets share the fixture in `common/src`.

## Persistent results

Every launcher creates the following directories under the repository root:

- `fuzzing/<tool>/corpus`: converted seeds and the mutable corpus discovered by libFuzzer.
- `fuzzing/<tool>/artifacts`: crash, timeout, leak, and other failure inputs.

The complete `/fuzzing/` tree is gitignored. Launchers run the fuzzer from
`fuzzing/<tool>`, so libFuzzer worker logs also stay there. Build products remain
in `cmake-build-fuzzing`, or in `ERBSLAND_CORE_FUZZ_BUILD_DIR` when overridden.
Deleting or replacing a build directory therefore leaves fuzzing results intact.
Subsequent launches reuse the corpus and refresh the checked-in seeds.

Arguments are forwarded to libFuzzer after the default corpus and artifact-prefix
arguments. Relative paths in those arguments are resolved from `fuzzing/<tool>`;
use absolute paths to refer to inputs elsewhere. Console diagnostics remain on
the terminal. Explicit libFuzzer output-path options can override these defaults.

To retain corpus files from an older build, copy them once from
`<old-build>/corpus/<tool>/` into `fuzzing/<tool>/corpus/` without overwriting
existing files. Launchers do not import old build directories automatically.

## HTTP/1.x request and response codecs

The transport-independent HTTP decoders have separate bounded state-machine targets with fixed-length, chunked,
malformed, and request-smuggling seeds:

```shell
test/fuzzing/http1-request/run.sh -max_total_time=60
test/fuzzing/http1-response/run.sh -max_total_time=60
```

The leading selector bytes control fragmentation and, for responses, HEAD/CONNECT method context. Both targets cap
wire input at 128 KiB and run with AddressSanitizer, UndefinedBehaviorSanitizer, and libFuzzer coverage.

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

Its first byte modulo seven selects raw pre-handshake transport input, authenticated post-handshake messages, alerts,
application data, raw established-state transport input, encrypted Certificate-handshake input, or encrypted
CertificateVerify-handshake input. The last two modes protect the mutated inner handshake with the actual negotiated
handshake keys rather than bypassing AEAD. CertificateVerify mode first parses and validates a real Ed25519 certificate;
the corpus includes that complete certificate and a correctly framed invalid signature. These modes do not bypass
authentication. The older post-handshake modes deliberately bypass certificate authentication to reach later states.
All modes feed bounded fragments and exercise parser, state, key-transition, queue, alert, and erasure paths.

## TLS 1.3 server protocol

From the repository root, start the bounded server state-machine fuzzer with:

```shell
test/fuzzing/tls-server-protocol/run.sh -max_total_time=60
```

Its first byte selects raw pre-checkpoint ClientHello transport input, authenticated post-handshake messages, alerts,
application data, or raw established-state transport input. The target embeds an Ed25519 test identity, establishes a
real client/server core pair for authenticated modes, and feeds fragments under the 64-KiB input cap. Both protocol
targets execute pending validation and signature requests explicitly before resuming. The shared public test identity
uses a fixed validation instant, so certificate expiry does not silently disable those paths over time.

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
its corpus in `fuzzing/x509-der/corpus`, and begins fuzzing. It
uses `/opt/local/bin/clang++-mp-22` by default. Override that compiler or the
build directory when needed:

```shell
CXX=/path/to/clang++ test/fuzzing/x509-der/run.sh -max_total_time=60
ERBSLAND_CORE_FUZZ_BUILD_DIR=/tmp/core-fuzz test/fuzzing/x509-der/run.sh -runs=1000
```

The launcher converts the checked-in `x509-der/corpus/example-certificate.pem`
into DER and adds it to the persistent mutable corpus on every run. This provides a
valid X.509 certificate from the first execution; add further PEM seed
certificates in that directory when they exercise distinct parser paths.

Arguments following the script name are forwarded to libFuzzer. Useful ones
include `-max_total_time=60` for a bounded run and `-runs=1000` for a quick
smoke test. Press Ctrl-C to stop an unbounded run.

To reproduce a crash, pass it directly to the built fuzzer executable:

```shell
cmake-build-fuzzing/test/fuzzing/x509-der/x509-der-fuzz fuzzing/x509-der/artifacts/crash-<hash>
```
