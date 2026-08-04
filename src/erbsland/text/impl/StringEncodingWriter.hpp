// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Char.hpp"
#include "../StringEncoding.hpp"
#include "../u16/impl/U16Writer.hpp"
#include "../u32/impl/U32Writer.hpp"
#include "../u8/impl/U8Writer.hpp"

namespace erbsland::text::impl {

/// Write Unicode characters in a selected binary string encoding.
/// @tested{StringEncoderTest}
template <typename tWriter>
class StringEncodingWriter final {
public:
    /// Create a string encoding writer over a sequential byte writer.
    StringEncodingWriter(tWriter &writer, const StringEncoding encoding) : _writer{writer}, _encoding{encoding} {
        _writer.setEndianness(encoding.endianness());
    }

public:
    /// Write one valid Unicode character or an encoding-boundary signal in the selected encoding.
    void write(const Char character) {
        if (character.isByteOrderMark()) {
            writeBomSignal();
            return;
        }
        switch (_encoding.effectiveEncoding().toRawValue()) {
        case StringEncoding::Utf8:
            U8Writer{_writer}.write(character);
            return;
        case StringEncoding::Utf16LittleEndian:
        case StringEncoding::Utf16BigEndian:
            U16Writer{_writer}.write(character);
            return;
        case StringEncoding::Utf32LittleEndian:
        case StringEncoding::Utf32BigEndian:
            U32Writer{_writer}.write(character);
            return;
        case StringEncoding::Utf16:
        case StringEncoding::Utf32:
            break;
        }
    }
    /// Write the byte order mark for the selected encoding.
    void writeBom() { write(Char::byteOrderMark()); }

private:
    /// Write the byte-order mark corresponding to the selected encoding.
    void writeBomSignal() {
        switch (_encoding.effectiveEncoding().toRawValue()) {
        case StringEncoding::Utf8:
            U8Writer{_writer}.writeBom();
            return;
        case StringEncoding::Utf16LittleEndian:
        case StringEncoding::Utf16BigEndian:
            U16Writer{_writer}.writeBom();
            return;
        case StringEncoding::Utf32LittleEndian:
        case StringEncoding::Utf32BigEndian:
            U32Writer{_writer}.writeBom();
            return;
        case StringEncoding::Utf16:
        case StringEncoding::Utf32:
            break;
        }
    }

private:
    tWriter &_writer;
    StringEncoding _encoding;
};

template <typename tWriter>
StringEncodingWriter(tWriter &, StringEncoding) -> StringEncodingWriter<tWriter>;

}
