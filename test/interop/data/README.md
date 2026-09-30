# Data interoperability

The pinned Rust counterpart uses bson 2.15.0, ciborium 0.2.2, and roxmltree 0.21.1.
Run its `emit` command to regenerate `vectors/rust-*`; the C++ suite reads these committed vectors and
writes its own BSON, CBOR, and XML files for Rust to verify. `Cargo.lock` pins the full dependency graph.

The fixture is a Turkish tempo variation: `aksak` at 96 bpm with a three-byte accent pattern.
