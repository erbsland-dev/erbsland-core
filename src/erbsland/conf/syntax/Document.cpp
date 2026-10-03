// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Document.hpp"

namespace erbsland::conf::syntax {

auto Document::tokensForLine(const unit::LineIndex line) const noexcept -> const util::List<TokenPtr> & {
    if (line.isNoIndex()) {
        return _lineTokens.getRef(unit::ItemIndex::noIndex());
    }
    return _lineTokens.getRef(unit::ItemIndex::fromSizeT(line.toSizeT()));
}

}
