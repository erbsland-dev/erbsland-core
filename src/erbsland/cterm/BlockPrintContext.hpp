// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Block_fwd.hpp"
#include "BlockAttributes.hpp"
#include "BlockPrintContext_fwd.hpp"
#include "BlockString_fwd.hpp"
#include "BlockStringEditor_fwd.hpp"
#include "BlockStyle.hpp"
#include "Color.hpp"

#include "../text/String.hpp"
#include "../text/StringEditor.hpp"
#include "../text/StringLiteral.hpp"
#include "../text/u32/U32String.hpp"
#include "../text/u32/U32StringEditor.hpp"
#include "../text/u32/U32StringLiteral.hpp"
#include "../text/u8/U8StringLiteral.hpp"

namespace erbsland::cterm {

/// The print context interface for terminal block print commands.
/// You only need this interface if you implement a custom cursor writer subclass that uses an unusual backend
/// storage. For regular writers, rely on the default implementation and implement the `write` methods.
/// @tested{CursorWriterTest BlockStringTest TerminalConvenienceTest}
class BlockPrintContext {
public:
    // defaults
    virtual ~BlockPrintContext() = default;

public:
    /// Commit the printed content and final active style.
    virtual void commit() noexcept = 0;

public: // main overloads
    /// Change the active color.
    virtual void print(Color color) noexcept = 0;
    /// Change the active foreground color.
    virtual void print(Foreground color) noexcept = 0;
    /// Change the active background color.
    virtual void print(Background color) noexcept = 0;
    /// Overlay the active style.
    virtual void print(BlockStyle style) noexcept = 0;
    /// Overlay the active attributes.
    virtual void print(BlockAttributes attributes) noexcept = 0;
    /// Print a terminal character.
    virtual void print(const Block &character) noexcept = 0;
    /// Print terminal text.
    virtual void print(const BlockStringEditor &text) noexcept = 0;
    /// Print terminal text.
    virtual void print(const BlockString &text) noexcept = 0;
    /// Print UTF-8 text.
    virtual void print(const text::String &text) noexcept = 0;
    /// Print UTF-32 text.
    virtual void print(const text::U32String &text) noexcept = 0;

public:
    /// Change the active foreground color.
    void print(const Foreground::Hue color) noexcept { print(Foreground{color}); }
    /// Change the active background color.
    void print(const Background::Hue color) noexcept { print(Background{color}); }
    /// Print UTF-32 text.
    void print(const text::U32StringEditor &text) noexcept { print(text::U32String{text}); }
    /// Print a UTF-8 literal.
    template <typename tChar>
    void print(const text::U8StringLiteral<tChar> &text) noexcept {
        print(text::String{text});
    }
    /// Print a UTF-32 literal.
    void print(const text::U32StringLiteral &text) noexcept { print(text::U32String{text}); }
};

}
