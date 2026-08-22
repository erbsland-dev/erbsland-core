# Network Interoperability Tests

This directory contains the standalone network interoperability executable. It uses Erbsland Unit Test for reporting
and selection, but it is deliberately not registered with CTest: the executable starts and controls subprocesses.

Enable it explicitly and run it manually:

```shell
cmake -S . -B cmake-build-interop -G Ninja -DCMAKE_BUILD_TYPE=Debug \
    -DERBSLAND_CORE_ENABLE_INTEROP_TESTS=ON
cmake --build cmake-build-interop --target erbsland-core-network-interop
cmake-build-interop/test/interop/network/erbsland-core-network-interop
```

On a shared filesystem that cannot host Cargo's incremental build artifacts, place the Rust target directory on a
guest-local filesystem:

```shell
cmake -S . -B cmake-build-interop -G Ninja -DCMAKE_BUILD_TYPE=Debug \
    -DERBSLAND_CORE_ENABLE_INTEROP_TESTS=ON \
    -DERBSLAND_CORE_CARGO_TARGET_DIRECTORY=/path/on/the/guest/network-interop-cargo
```

## Architecture

Before the test controller runs, `main` opens an ephemeral TCP listener on IPv4 loopback and starts the Rust
counterpart through `system::Subprocess`. The counterpart connects a permanent unencrypted control channel and
authenticates its protocol version and per-run random token. Commands and responses are bounded UTF-8 JSONL messages.

Each `start` command creates a fresh scenario listener on an ephemeral loopback port. The C++ test receives that port,
uses the public asynchronous network API under test, and then sends `finish` to join the scenario and collect its
result. A scenario handle sends `cancel` during unwinding. `shutdown` joins remaining scenarios and exits the
counterpart; control-channel EOF also terminates it. Finally, `Subprocess` ownership guarantees termination and reaping
if orderly shutdown fails.

The control plane uses native sockets rather than Erbsland Network, keeping orchestration independent of the code being
tested. The Rust counterpart uses rustls with the ring provider and no async runtime. It also exposes bounded
`http-request`, `http-response`, and `http-chunk-size` comparison commands backed by the independent `httparse` crate;
the C++ side compares acceptance and exact parsed control/header values with the Core codecs.

Deterministic `http_plain_server` and `http_tls_server` scenarios independently parse a fixed POST with `httparse`,
validate its target, Host field, framing, and body, then return an informational head followed by chunked JSON and
trailers. The TLS mode uses rustls, the test certificate, and HTTP/1.1-only ALPN.

## Protocol

Protocol version 1 has a 64 KiB maximum line length. Every request and response carries `protocol` and numeric `id`.
The commands are:

- `start`: select a named scenario, optional cipher, and payload length; returns a scenario ID and ephemeral port.
- `finish`: join a completed scenario and return its byte count.
- `cancel`: interrupt and join a scenario; safe to send during test failure cleanup.
- `shutdown`: cancel remaining scenarios and terminate the counterpart.

New scenarios belong in the Rust dispatcher and should expose only stable, machine-readable parameters and results.
Tests should keep protocol assertions in C++ so they exercise the public Core implementation.

The matrix exercises Core in both TLS client and accepted-TLS server roles. It covers all supported TLS 1.3 cipher
suites, RSA/ECDSA/Ed25519 server identities, exact and default SNI selection, TLS 1.2 rejection, an unsupported selected
cipher, handshake interruption, KeyUpdate, graceful and truncated closure, ALPN, and four-MiB duplex streams.
