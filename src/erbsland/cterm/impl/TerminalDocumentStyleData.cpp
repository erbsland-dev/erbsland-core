// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TerminalDocumentStyleData.hpp"

namespace erbsland::cterm::impl {

TerminalDocumentStyleData::TerminalDocumentStyleData() :
    baseTextStyle{Color{fg::White, bg::Black}},
    baseBlockLayout{0, ParagraphIndents::cUseLineIndent, ParagraphIndents::cUseLineIndent, bgeo::BlockMargins{0}} {
}

}
