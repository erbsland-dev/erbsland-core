// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CompressionWorkerWorkload_fwd.hpp"
#include "ProfileTypes.hpp"

#include <erbsland/compression/ByteCompressor.hpp>
#include <erbsland/compression/ByteDecompressor.hpp>
#include <erbsland/compression/CompressionAlgorithm.hpp>
#include <erbsland/compression/CompressionFormat.hpp>
#include <erbsland/compression/CompressionLevel.hpp>
#include <erbsland/compression/DecompressionOptions.hpp>
#include <erbsland/profiling/WorkerWorkload.hpp>

#include <memory>

namespace app::compression {

/// An independent one-shot compression or decompression worker.
/// @notest{Covered by the compression profiling smoke tests.}
class CompressionWorkerWorkload final : public erbsland::profiling::WorkerWorkload {
public:
    /// Create a worker for one fully prepared scenario.
    CompressionWorkerWorkload(
        ProfileOperation operation,
        erbsland::compression::CompressionAlgorithm algorithm,
        erbsland::compression::CompressionFormat format,
        erbsland::compression::CompressionLevel level,
        erbsland::compression::DecompressionOptions decompressionOptions,
        std::shared_ptr<const erbsland::ByteBlock> source,
        std::shared_ptr<const erbsland::ByteBlock> compressed);

public: // implement WorkerWorkload
    [[nodiscard]] auto execute(const erbsland::profiling::WorkerExecutionContext &context)
        -> erbsland::profiling::WorkerMeasurement override;

private:
    ProfileOperation _operation;                                            ///< Operation measured by this worker.
    std::unique_ptr<erbsland::compression::ByteCompressor> _compressor;     ///< Configured compressor, if used.
    std::unique_ptr<erbsland::compression::ByteDecompressor> _decompressor; ///< Configured decompressor, if used.
    std::shared_ptr<const erbsland::ByteBlock> _source;                     ///< Shared uncompressed input.
    std::shared_ptr<const erbsland::ByteBlock> _compressed;                 ///< Shared compressed input.
};

}
