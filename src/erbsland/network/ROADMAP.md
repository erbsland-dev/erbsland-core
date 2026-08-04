# Network Roadmap: Event-Driven TLS and HTTP

## Goal

Build a small, portable, event-driven network framework that can run an HTTPS web server and make HTTP or HTTPS
client requests without external runtime dependencies.

The first complete framework shall provide:

- a public TLS 1.3 connection layer for outgoing and accepted TCP connections;
- explicit handshake checkpoints where application policy can inspect progress and abort before the next important
  handshake step;
- an HTTP/1.1 server that binds to a local endpoint and dispatches requests without blocking its event loop;
- Flask-like path-handler registration, ranging from one-line text responses to streamed custom responses;
- secure static-file mounts with URL-prefix mapping, ordered directory lookup, and deterministic priorities; and
- an HTTP/HTTPS client suitable for REST requests, redirects, bounded response aggregation, and streamed downloads.

This roadmap describes intended capabilities and delivery order. Names used for future types and callbacks are
illustrative, not a frozen public API. New public names must be checked against the API guidelines before implementation.

## Scope of the First Complete Framework

The first target is HTTP/1.1 over TCP or TLS 1.3. It must support persistent connections, bounded request and response
bodies, streaming with back-pressure, and graceful cancellation. It does not need HTTP/2 or HTTP/3 to be useful and
secure.

The initial TLS profile remains deliberately narrow:

- TLS 1.3 only;
- X25519 key exchange;
- AES-128-GCM/SHA-256, AES-256-GCM/SHA-384, and ChaCha20-Poly1305/SHA-256;
- certificate authentication, SNI, and ALPN;
- no PSK, session resumption, 0-RTT, HelloRetryRequest, or post-handshake authentication; and
- no legacy SSL or TLS 1.0--1.2 compatibility.

The initial HTTP profile includes normal HTTP/1.1 framing, fixed-length and chunked bodies, connection-close framing,
informational responses where required for interoperability, and explicit handling of methods with no response body.
WebSockets, HTTP/2, HTTP/3, server-side templates, CGI, compression, caching proxies, and a browser-style cookie jar are
follow-up features.

## Architecture Boundaries

### Event-loop ownership

Every public network, TLS, and HTTP source belongs to one `event::Events` target. Its mutable state and callbacks stay
on that owner loop. Cross-thread work returns by posting an event; it never invokes user code from a worker or native I/O
completion thread.

Public operations remain non-blocking and use the existing atomic send contract: accept the complete submitted block,
return back-pressure without partial ownership transfer, or report that output is closed. Every queued byte, message,
request, connection, timer, and file operation has a configured bound.

### TCP, TLS, and HTTP are separate layers

`TcpConnection` remains the unauthenticated byte-stream transport. The public TLS connection shall be a sibling API,
implemented by composition over a TCP connection rather than derived from it. It may resemble the TCP API, but it must
not report an active application stream merely because TCP connected, and it must not expose unauthenticated handshake
bytes as application data.

HTTP operates on an authenticated or plaintext connection abstraction with the same bounded send, receive, pause,
resume, close, and abort capabilities. HTTP parsing must not know whether its bytes came from TCP or TLS. TLS-specific
metadata remains accessible through the secure connection, not through generic HTTP message objects.

### Handshake checkpoints

Important TLS transitions run as ordered owner-loop checkpoints. The protocol stops before crossing the checkpoint,
invokes the configured callback, and observes an abort before processing or transmitting the next flight. A callback
may inspect only authenticated facts appropriate to that stage.

The first API needs checkpoints for:

1. TCP transport establishment;
2. peer hello processing and selected protocol parameters;
3. server-side ClientHello policy, including SNI identity and ALPN selection;
4. client-side peer-certificate validation;
5. optional peer-authentication policy when client certificates are added; and
6. completion of the authenticated handshake.

Application data cannot be sent or delivered before handshake completion. Terminal error and final callbacks remain
exactly-once events even if a checkpoint aborts, a deadline fires, and the transport closes concurrently.

### HTTP transactions own body flow

Headers and body data are separate events. Receiving headers never implies that the body was aggregated. A handler or
client may choose bounded aggregation, incremental body callbacks, streaming into a sink, or rejection before reading
the body. Pausing a body must propagate back-pressure through HTTP, TLS, and TCP without unbounded buffering.

## Current Capabilities

This is an inventory of foundations available to the network work, not a history of completed cryptology milestones.
The detailed steps from the former cryptology roadmap are intentionally not carried forward.

### Event and TCP

- `Network` provides event-loop-owned DNS lookup, TCP listener, TCP connection, and UDP socket factories.
- TCP supports outgoing resolution and connection, transferable pending accept requests, shared accepted-connection
  quotas, pause/resume, bounded queues, atomic sends, writable notifications, graceful close, abort, deadlines, and
  structured errors. A quota lease follows an accepted socket through its request into the active connection.
- Native TCP and listener devices are available for the supported macOS, Linux, and Windows targets.

### TLS client protocol and cryptology

- SHA-256/SHA-384, HMAC, HKDF, protected secret storage, X25519, TLS record protection, and supported public-key
  signature verification are available.
- Strict DER/PEM X.509 parsing and explicit-anchor server-certificate validation cover chain, purpose, time, DNS/IP
  identity, critical extensions, and bounded path construction.
- The transport-independent `TlsClientProtocol` implements the authenticated non-PSK TLS 1.3 client handshake,
  application records, alerts, KeyUpdate, timeout, truncation detection, and bidirectional `close_notify`.
- The sibling transport-independent `TlsServerProtocol` implements the authenticated non-PSK TLS 1.3 server handshake,
  bounded ClientHello policy checkpoints, exact canonical SNI identity selection with a required default, ALPN,
  application records, alerts, KeyUpdate, timeout, truncation detection, and bidirectional `close_notify`.
- `CryptologyConfiguration` provides an application-wide registry of immutable `TlsConfiguration` snapshots with
  bounded hierarchical labels and exact-to-global fallback resolution.
- `TlsClientConnection` composes the client protocol over `TcpConnection`, exposes authenticated lifecycle checkpoints,
  preserves bounded back-pressure, and applies independent handshake, application-idle, and graceful-close deadlines.
- `TlsServerConnection` composes the server protocol over an accepted `TcpConnection`, captures labeled immutable
  identities before consuming the request, and separately quotas incomplete handshakes.
- TLS wire readers, writers, record deframing, handshake deframing, transcript handling, and the TLS 1.3 key schedule
  are reusable by a server protocol.
- Unit vectors, malformed-input tests, structured fuzzing, and macOS/Linux/Windows validation cover the current client
  core.

### Important gaps

- There is no HTTP message model, HTTP/1.1 codec, HTTP server, or HTTP client.
- Existing path streams are not by themselves permission to perform blocking filesystem operations on an event-loop
  thread. Static-file service needs an explicitly event-safe file-open/read pump.
- Certificate validation currently uses explicit anchors. Platform trust-store integration is still required for a
  convenient general-purpose HTTPS client.

## Delivery Plan

Each increment below should leave the repository buildable, documented, and independently testable. Do not combine two
increments merely to avoid exposing a small internal seam; narrow seams make protocol review and fuzzing easier.

### 1. Freeze connection-layer lifecycle and limits — Complete

Define the shared behavioral contract before adding public TLS types:

- Specify callback ordering, reentrancy, owner-loop affinity, finalization, cancellation, and exactly-once terminal
  events for TCP-derived layers.
- Define distinct transport-connect, handshake, active-stream, closing, closed, and failed states.
- Define how pause/resume and writable transitions propagate through a composed connection.
- Separate connect, handshake, idle, and graceful-close deadlines. No timeout silently resets because unrelated bytes
  arrived.
- Establish defaults and hard maxima for transport queues, decrypted queues, pending events, and bytes accepted in one
  public send.
- Decide the minimal metadata exposed at each TLS checkpoint and ensure later events cannot retroactively invalidate
  facts reported as authenticated.

Completion gate: deterministic tests drive every legal state transition plus abort, timeout, EOF, and back-pressure at
each transition.

Status: **Complete.** The connection contract, independent deadlines, checkpoint behavior, bounded queues, and
exactly-once terminal sequencing are implemented and covered by deterministic connection and protocol tests.

### 2. Public event-driven TLS client connection — Complete

Wrap the completed client protocol in a public, one-shot secure connection:

- Own a `TcpConnection`, retain the original unresolved `Host`, and feed transport input/output without copying secret
  state into the facade.
- Expose host resolution, transport connection, negotiated parameters, accepted peer identity, handshake completion,
  authenticated data, writable, graceful close, error, and final events.
- Pause protocol progression at the defined checkpoints so `abort()` takes effect before the next handshake step.
- Translate TCP EOF and closure into TLS `close_notify` or truncation semantics and drain an accepted close alert before
  closing TCP.
- Apply independent connect, handshake, idle, and close deadlines.
- Preserve bounded queues in both directions when TCP and TLS record sizes differ.

Completion gate: mock-transport tests fragment and coalesce every boundary, force back-pressure at every output, and
abort at every checkpoint. Live tests connect to at least two independent TLS 1.3 implementations on macOS, Linux, and
Windows.

Status: **Complete.** The public facade and labeled application configuration are implemented. Deterministic coverage
passes on every supported platform. A standalone, subprocess-capable interoperability framework uses a Rust/rustls
counterpart to cover the three supported TLS 1.3 suites, protocol and cipher rejection, interrupted handshakes,
orderly and truncated closure, and multi-megabyte streams without placing process launches in the unit tests.

### 3. Protected server identity and signing – Complete

Add only the private-key capabilities required by the first TLS server:

- Introduce a move-only protected signing-key abstraction with no raw private-key accessor.
- Parse a strictly bounded PKCS#8 key supplied as DER or PEM. Encrypted-at-rest key-container support can follow; the
  initial limitation must be explicit in API documentation.
- Implement one small interoperable signing path first, preferably Ed25519, then add ECDSA P-256 and RSA-PSS according
  to deployment demand.
- Match a configured private key to the leaf certificate before listening and select only a signature scheme supported
  by both the key and ClientHello.
- Keep decoded scalar material, deterministic signing intermediates, and temporary plaintext key data in visible,
  bounded lifetimes with immediate erasure.

Cryptographic implementation remains specification ordered: API documentation names the governing specification and
section, and inline comments map every substantive step from specification notation to code variables.

Completion gate: specification vectors, generated certificate/key pairs, altered-key rejection, fault paths, erasure
checks, cross-platform tests, and structured fuzzing all pass. Run the anti-pattern utility after each signing design
increment rather than waiting for pre-commit.

Status: **Complete.** Strict protected PKCS#8 loading, Ed25519, deterministic ECDSA P-256, RSA-PSS signing, semantic
leaf-certificate matching, immutable shared server identities, structured fuzzing, and cross-platform unit coverage are
implemented. The TLS server protocol remains the next independent increment.

### 4. Transport-independent TLS 1.3 server protocol — Complete

Build a sibling to `TlsClientProtocol` while reusing the existing wire, transcript, schedule, and record components:

- Parse a bounded ClientHello and strictly validate offered versions, cipher suites, compression, signature schemes,
  supported groups, key shares, SNI, and ALPN.
- Select one configured identity, one supported cipher suite, X25519, and optionally `http/1.1` ALPN.
- Produce ServerHello, EncryptedExtensions, Certificate, CertificateVerify, and Finished in RFC 8446 order.
- Install handshake and application traffic keys exactly at their record boundaries.
- Support application records, alerts, KeyUpdate, orderly closure, timeout, truncation detection, and back-pressure.
- Reject PSK, early data, unsupported groups, and HelloRetryRequest-dependent clients explicitly.
- Start with one configured identity. Add SNI-based identity selection as a separate increment after the single-identity
  path is interoperable.

Completion gate: a deterministic in-memory client/server handshake exercises every cipher suite and legal fragmentation
pattern. Negative tests cover malformed ClientHello, downgrade and algorithm confusion, wrong keys, bad Finished,
unexpected messages, limits, and checkpoint aborts. A bounded server-state fuzz target is clean under ASan/UBSan.

Status: **Complete.** The internal single-identity server core, strict bounded ClientHello negotiation, atomic server
flight, exact traffic-key transitions, application lifecycle, deterministic cross-role tests, RSA-PSS/ECDSA/Ed25519
identity coverage, negative matrices, erasure checks, and bounded authenticated-state fuzz target are implemented.
Focused tests pass on macOS, Linux, and Windows, and every staged anti-pattern and pre-commit gate is clean.

### 5. Public accepted TLS connections — Complete

Integrate the server protocol into the event-driven TCP accept path:

- Accept or reject a `TcpConnectionRequest` without exposing the accepted socket to application data handlers.
- Configure certificate identity, ALPN policy, handshake timeout, and buffer limits before starting the handshake.
- Emit a ClientHello policy checkpoint containing bounded SNI, ALPN, and negotiation metadata.
- Add deterministic SNI identity selection and ALPN selection without blocking the event loop.
- Expose an authenticated secure byte stream only after Finished is verified.
- Bound concurrent handshakes separately from established connections so incomplete handshakes cannot exhaust the
  listener.

Completion gate: a loopback TLS client and server exchange streamed data, update keys, close from either side, and
survive forced back-pressure. OpenSSL and curl interoperability tests pass on all supported platforms.

Status: **Complete.** The reusable thread-safe `ConnectionQuota`, private listener defaults, shared multi-listener
capacity, endpoint-aware leases, listener/request/connection lease transfer, and release wakeups are implemented.
The public accepted TLS facade provides exact canonical SNI identity mappings with a required default, server-preference
ALPN, ClientHello and authenticated checkpoints, bounded stream flow, separate handshake quotas and deadlines, and
exactly-once terminal cleanup.

The complete deterministic test suite passes on macOS, Linux, and Windows. Standalone Rust/rustls interoperability
passes on all three platforms for both client and server roles, all three cipher suites, RSA/ECDSA/Ed25519 identities,
default and exact SNI selection, ALPN, KeyUpdate, truncation, graceful closure, and four-MiB duplex streams. The
standalone OpenSSL `s_client` and curl HTTPS tests pass on Linux and Windows, including streamed application data and a
manually emitted HTTP/1.1 response.

On macOS the same external tests pass with the explicitly selected MacPorts OpenSSL 3.6.3 and curl 8.21.0 build linked
against OpenSSL 3.6.3. The anti-pattern scan and complete pre-commit workflow pass. The remaining HTTP value types in
milestone 6 are the next increment.

### 6. URL, header, and HTTP value types

Add protocol-independent values before writing the HTTP parser:

- Parse absolute network URLs with scheme, host, optional port, decoded NFC path, query, and fragment.
- Reuse `HostName` and strict IDNA handling; support bracketed IPv6 literals and default ports.
- Resolve relative redirect references against a base URL without lossy string concatenation.
- Canonically percent-encode decoded URL components; original escape spelling is intentionally not retained.
- Add validated HTTP methods, status codes, versions, media types, and ordered multi-value headers.
- Reject CR/LF injection, invalid field names, forbidden control characters, and values beyond configured limits at
  construction boundaries.
- Treat field names case-insensitively while preserving values and repeated fields such as `Set-Cookie`.

Completion gate: RFC examples, canonicalization boundaries, IDNA hosts, IPv6 authorities, relative redirects, duplicate
headers, injection attempts, and limit behavior have focused unit tests.

**Status:** URL is already implemented. HTTP methods, status codes, versions, media types, and ordered multi-value
headers are the next implementation increment after milestone 5 validation closes.

### 7. Strict incremental HTTP/1.1 codecs

Implement request and response framing following RFC 9110 and RFC 9112:

- Parse and serialize request lines, status lines, headers, fixed-length bodies, chunked bodies, trailers, and
  connection-close-delimited responses incrementally across arbitrary byte boundaries.
- Reject obsolete line folding, conflicting or malformed Content-Length, ambiguous Transfer-Encoding, invalid chunk
  syntax, forbidden trailer fields, and bytes after a completed message where the connection state disallows them.
- Model HEAD, 1xx, 204, 304, CONNECT, and request-method-dependent response-body rules explicitly.
- Enforce configurable start-line, header count, aggregate header bytes, chunk metadata, trailer, and body limits before
  allocation.
- Keep parsing and serialization transport independent and free of callbacks so they can be fuzzed exhaustively.

Completion gate: differential tests against an independent implementation, request-smuggling regression cases,
byte-by-byte fragmentation tests, pipelined-message tests, and bounded request/response fuzz targets pass.

### 8. One event-driven HTTP transaction over a connection

Connect the HTTP codecs to either a TCP or TLS application stream:

- Deliver request or response headers first, followed by bounded body chunks and one completion event.
- Allow the consumer to aggregate up to an explicit maximum, stream chunks, pause, resume, or reject the remaining
  body.
- Propagate send back-pressure to body producers through writable events.
- Support one transaction at a time initially; queueing and connection reuse come later.
- Define timeout phases for headers, body idle time, total transaction time, and graceful close.
- Prevent callbacks belonging to a completed or cancelled transaction from observing a reused connection.

Completion gate: the same transaction tests run over mock plaintext and mock TLS streams with forced fragmentation,
back-pressure, cancellation, timeout, and early close.

### 9. Minimal HTTP server and routing

Create an event source that owns a listener and active connections:

- Bind to an `IpEndpoint`, report the effective port, pause/resume accepting, close gracefully, and abort immediately.
- Support plaintext HTTP and HTTPS listener configurations through the same request-routing layer.
- Register handlers by HTTP method and URL path using a small Flask-like model.
- Start with exact paths, then add named single-segment parameters and a final catch-all parameter.
- Define routing precedence independent of registration accidents: exact paths, parameterized paths, catch-all paths,
  then explicit fallback handlers. Equal-specificity duplicates are configuration errors.
- Give each request a response transaction that supports simple text/byte replies, status selection, custom headers,
  content type, fixed bodies, and streamed bodies.
- Automatically produce bounded 400, 404, 405, 408, 413, 431, and 500 responses where application code has not
  already committed a response.
- Keep handler execution on the owner loop and permit a handler to retain the response transaction for later event-based
  completion without retaining unrelated request-body buffers.

Completion gate: a small application can bind an ephemeral port, register health and parameterized REST handlers,
receive streamed request bodies, return custom and streamed responses, use keep-alive sequentially, and shut down with
all callbacks finalized exactly once.

### 10. Event-safe static-file service

Add static files as a routing target rather than a special blocking handler:

- Register a URL prefix with one or more filesystem roots. Search roots in declared priority order and serve the first
  eligible regular file.
- Support multiple mounts with explicit priorities and deterministic registration-order tie breaking.
- Strip the matched URL prefix, percent-decode exactly once, normalize separators, and reject empty forbidden segments,
  NUL, drive changes, absolute paths, `.`/`..`, encoded traversal, and platform-specific separator tricks.
- Resolve and open beneath the configured root using a documented symlink policy. The default must prevent escaping the
  root even when links or directories change concurrently.
- Perform metadata lookup, open, and bounded chunk reads outside the event-loop thread through a native asynchronous
  backend or bounded worker bridge. Return completions to the owning event loop.
- Stream file chunks according to connection back-pressure; never aggregate a large file in memory.
- Provide extension-based media types with an explicit fallback, `Content-Length`, HEAD support, and configurable index
  files. Add conditional and range requests as a follow-up increment after basic streaming is correct.
- Bound open files, concurrent reads, queued file bytes, and per-connection static-file work.

Completion gate: priority overlays, mappings, missing files, MIME types, empty files, large files, slow clients,
cancellation, symlink races, encoded traversal, Unicode names, and Windows/POSIX path differences are tested. Event-loop
latency remains bounded while large files are served concurrently.

### 11. HTTP/HTTPS client

Build a request source around an absolute URL:

- Resolve the URL, connect TCP, negotiate TLS for `https`, and generate the Host header from the original authority.
- Support common methods, ordered headers, empty or fixed request bodies, and streamed request bodies with
  back-pressure.
- Emit response headers, body chunks, writable transitions for upload producers, completion, error, and final events.
- Provide an explicit bounded convenience operation for small text or byte responses without changing the streaming
  default.
- Permit response streaming into an application sink or event-safe file writer for large downloads.
- Separate DNS, TCP, TLS, response-header, body-idle, and overall deadlines in diagnostics.
- Use explicit trust anchors first, then add platform trust-store policies before calling the client generally
  convenient for public HTTPS URLs.

Completion gate: GET, HEAD, POST, fixed and chunked uploads, fixed/chunked/close-delimited downloads, informational
responses, empty bodies, cancellation, limits, and TLS failures pass against deterministic servers and independent HTTP
servers.

### 12. Redirects, reuse, and download policy

Add convenience behavior only after one request is reliable:

- Follow 301, 302, 303, 307, and 308 using explicit method/body rewrite rules and relative URL resolution.
- Bound redirect count, detect loops, and make HTTPS-to-HTTP downgrade disabled by default.
- Strip credentials and origin-bound authorization headers on cross-origin redirects.
- Require replayable request bodies before automatically following a redirect that preserves the body.
- Reuse one keep-alive connection for sequential same-origin requests, then add a bounded per-origin pool if measured
  workloads justify it.
- Validate a reused connection before assigning a transaction and retry only operations that are explicitly safe and
  replayable.
- Add atomic download-to-temporary-file and replace-on-success as an optional sink without hiding progress or
  cancellation events.

Completion gate: redirect matrices, cross-origin header stripping, downgrade rejection, loop limits, non-replayable
bodies, stale keep-alive connections, partial downloads, and atomic destination behavior are covered by tests.

### 13. Hardening, interoperability, documentation, and demos

Before declaring the first framework complete:

- Run all TLS and HTTP parser/state-machine fuzz targets under ASan and UBSan with stable seed corpora.
- Exercise slowloris behavior, queue exhaustion, connection floods, malformed records, request smuggling, response
  splitting, path traversal, symlink races, cancellation races, and teardown under back-pressure.
- Validate macOS, Linux, and Windows builds plus live interoperability with current OpenSSL, curl, and at least one
  browser-facing HTTPS path.
- Add reference documentation for every public type and a topic explaining event-loop ownership, lifetimes,
  back-pressure, TLS checkpoints, static mounts, and client streaming.
- Add a minimal HTTPS server demo with a health route, parameterized route, static-directory overlay, streamed response,
  and graceful shutdown.
- Add an HTTP client demo covering a REST request, redirects, bounded text response, and streamed file download.
- Run `utilities/run.py anti_patterns` after every larger design or implementation increment and the complete
  `pre_commit` workflow before each merge.

## Security and Resource Rules

These rules apply to every increment, not just the final hardening phase:

- Parse attacker-controlled bytes incrementally and enforce limits before allocation, recursion, queueing, or expensive
  cryptographic work.
- Never deliver unauthenticated TLS plaintext, partially parsed HTTP headers, or a filesystem path that has not passed
  containment checks.
- Make ownership transfer, secret creation, erasure, callback lifetime, cancellation, and terminal-state changes
  visible where they occur.
- Keep cryptographic code in specification order with governing specifications and sections in API documentation and
  step-level notation comments in implementations.
- Use constant-time operations for secrets and authenticators; document intentionally variable-time processing of
  public inputs.
- Treat protocol errors, policy rejection, timeouts, transport errors, TLS alerts, HTTP errors, and application aborts
  as distinct diagnostic categories.
- Redact authorization, cookies, private keys, traffic secrets, and sensitive body data from default diagnostics.
- Never resolve a static URL by concatenating strings or by checking a path before opening it without a containment-safe
  open strategy.
- Do not block an event-loop thread for DNS, socket I/O, file I/O, certificate policy, or application work advertised
  as asynchronous.

## Definition of the First Complete Framework

The milestone is complete when one application can, using only public APIs:

1. configure a certificate and protected private key, bind an HTTPS server, and observe every connection through
   ordered event callbacks;
2. register simple and parameterized handlers, read bounded or streamed request bodies, and send fixed or streamed
   custom responses;
3. mount prioritized directory overlays below URL prefixes and serve large files without blocking the event loop or
   buffering the complete file;
4. issue HTTP and HTTPS URL requests, follow a safe bounded redirect policy, process response headers, aggregate a small
   response, or stream a large response to a sink;
5. sustain sequential keep-alive traffic with bounded memory and back-pressure from file or application endpoints to
   the native socket; and
6. pass the cross-platform, fuzzing, security, interoperability, documentation, and demo gates above.

Reaching this milestone does not imply a browser engine or a general-purpose reverse proxy. It establishes a secure,
portable foundation on which HTTP/2, WebSockets, richer routing, cookies, caching, compression, client certificates,
session resumption, and platform-specific acceleration can be added as measured follow-up work.
