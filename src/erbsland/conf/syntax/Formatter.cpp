// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Formatter.hpp"

#include "impl/Writer.hpp"

namespace erbsland::conf::syntax {

auto Formatter::format(const DocumentPtr &document) const -> FormatResult {
    return impl::Writer{}.write(document, _format);
}

}
