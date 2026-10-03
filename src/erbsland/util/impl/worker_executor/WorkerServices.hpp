#pragma once

#include "WorkerExecutor.hpp"

/// Shared execution domains with dependency isolation, not independent worker implementations.
/// Coroutine and blocking network work may wait for streams, so native stream I/O has its own domain.
namespace erbsland::util::impl::worker_services {

/// Access the domain for DNS, HTTP blocking operations, trust evaluation and native signing.
/// @tested{WorkerExecutorTest CoTaskTest BufferedStreamTest HostLookupTest}
[[nodiscard]] auto blocking() -> WorkerExecutor &;
/// Access the independent domain that fills and drains stream buffers.
/// @tested{WorkerExecutorTest CoTaskTest BufferedStreamTest HostLookupTest}
[[nodiscard]] auto io() -> WorkerExecutor &;
/// Access the independent domain for coroutine work that may wait for complete stream operations.
/// @tested{WorkerExecutorTest CoTaskTest BufferedStreamTest HostLookupTest}
[[nodiscard]] auto coroutine() -> WorkerExecutor &;

}
