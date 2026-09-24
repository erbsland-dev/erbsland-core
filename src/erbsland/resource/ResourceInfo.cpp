// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ResourceInfo.hpp"

#include <utility>

namespace erbsland::resource {

ResourceInfo::ResourceInfo(
    const unit::ByteLength originalSize,
    const unit::ByteLength storedSize,
    std::optional<compression::CompressionAlgorithm> compressionAlgorithm,
    std::optional<cryptology::HashAlgorithm> hashAlgorithm,
    mem::ByteBlock hash,
    const bool encrypted) noexcept :
    _originalSize{originalSize},
    _storedSize{storedSize},
    _compressionAlgorithm{std::move(compressionAlgorithm)},
    _hashAlgorithm{std::move(hashAlgorithm)},
    _hash{std::move(hash)},
    _encrypted{encrypted} {
}

}
