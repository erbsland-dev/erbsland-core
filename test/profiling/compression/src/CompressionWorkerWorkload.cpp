// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CompressionWorkerWorkload.hpp"

#include <erbsland/profiling/WorkerExecutionContext.hpp>

#include <utility>

namespace app::compression {

namespace el = erbsland;
namespace pf = erbsland::profiling;

CompressionWorkerWorkload::CompressionWorkerWorkload(
    const ProfileOperation operation,
    const el::compression::CompressionAlgorithm algorithm,
    const el::compression::CompressionFormat format,
    const el::compression::CompressionLevel level,
    el::compression::DecompressionOptions decompressionOptions,
    std::shared_ptr<const el::ByteBlock> source,
    std::shared_ptr<const el::ByteBlock> compressed) :
    _operation{operation}, _source{std::move(source)}, _compressed{std::move(compressed)} {
    if (_operation == ProfileOperation::Compress) {
        _compressor = std::make_unique<el::compression::ByteCompressor>(algorithm, format, level);
    } else {
        _decompressor =
            std::make_unique<el::compression::ByteDecompressor>(algorithm, format, std::move(decompressionOptions));
    }
}

auto CompressionWorkerWorkload::execute(const pf::WorkerExecutionContext &context) -> pf::WorkerMeasurement {
    auto result = el::ByteBlock{};
    auto operations = std::uint64_t{};
    for (; operations < context.operations && !context.stopToken.stop_requested(); ++operations) {
        if (_operation == ProfileOperation::Compress) {
            result = _compressor->compress(*_source);
        } else {
            result = _decompressor->decompress(*_compressed);
        }
    }
    return pf::WorkerMeasurement{
        .operations = operations,
        .metrics =
            el::List<std::uint64_t>{
                operations * _source->length().toRawValue(), operations * _compressed->length().toRawValue()},
        .sink = result.length().toRawValue(),
        .digest = std::move(result)};
}

}
