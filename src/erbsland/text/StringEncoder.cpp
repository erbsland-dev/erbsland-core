// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringEncoder.hpp"

#include "impl/StringConversionTools.hpp"
#include "impl/StringEncodingWriter.hpp"

#include "../mem/ByteWriter.hpp"
#include "../mem/impl/RingBufferWriter.hpp"
#include "../mem/RingBuffer.hpp"

namespace erbsland::text {

namespace {

[[nodiscard]] auto normalizedCharacter(const Char character) noexcept -> Char {
    return character.isValidUnicode() ? character : Char::replacement();
}

}

#define ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(TYPE)                                                                    \
    auto StringEncoderTraits<TYPE>::encode(                                                                            \
        const TYPE &source, const StringEncoding encoding, const StringBomMode bomMode) -> mem::ByteBlock {            \
        return impl::StringConversionTools::encode(source, encoding, bomMode);                                         \
    }

ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(U8StringEditor);
ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(U8String);
ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(U16StringEditor);
ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(U16String);
ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(U32StringEditor);
ERBSLAND_DEFINE_STRING_ENCODER_TRAITS(U32String);

#undef ERBSLAND_DEFINE_STRING_ENCODER_TRAITS

#define ERBSLAND_DEFINE_STRING_ENCODER_LENGTH_TRAITS(TYPE)                                                             \
    auto StringEncoderTraits<TYPE>::encodedLength(                                                                     \
        const TYPE &source, const StringEncoding encoding, const StringBomMode bomMode) -> unit::ByteLength {          \
        return impl::StringConversionTools::encodedLength(source, encoding, bomMode);                                  \
    }

ERBSLAND_DEFINE_STRING_ENCODER_LENGTH_TRAITS(U8StringEditor);
ERBSLAND_DEFINE_STRING_ENCODER_LENGTH_TRAITS(U8String);
ERBSLAND_DEFINE_STRING_ENCODER_LENGTH_TRAITS(U16StringEditor);
ERBSLAND_DEFINE_STRING_ENCODER_LENGTH_TRAITS(U16String);
ERBSLAND_DEFINE_STRING_ENCODER_LENGTH_TRAITS(U32StringEditor);
ERBSLAND_DEFINE_STRING_ENCODER_LENGTH_TRAITS(U32String);

#undef ERBSLAND_DEFINE_STRING_ENCODER_LENGTH_TRAITS

#define ERBSLAND_DEFINE_STRING_ENCODER_TO_TRAITS(TYPE)                                                                 \
    auto StringEncoderTraits<TYPE>::encodeTo(                                                                          \
        const TYPE &source, mem::RingBuffer &buffer, const StringEncoding encoding, const StringBomMode bomMode)       \
        -> util::Result {                                                                                              \
        return impl::StringConversionTools::encodeTo(source, buffer, encoding, bomMode);                               \
    }

ERBSLAND_DEFINE_STRING_ENCODER_TO_TRAITS(U8StringEditor);
ERBSLAND_DEFINE_STRING_ENCODER_TO_TRAITS(U8String);
ERBSLAND_DEFINE_STRING_ENCODER_TO_TRAITS(U16StringEditor);
ERBSLAND_DEFINE_STRING_ENCODER_TO_TRAITS(U16String);
ERBSLAND_DEFINE_STRING_ENCODER_TO_TRAITS(U32StringEditor);
ERBSLAND_DEFINE_STRING_ENCODER_TO_TRAITS(U32String);

#undef ERBSLAND_DEFINE_STRING_ENCODER_TO_TRAITS

auto StringEncoderTraits<Char>::encode(const Char &source, const StringEncoding encoding, const StringBomMode bomMode)
    -> mem::ByteBlock {
    auto writer = mem::ByteWriter{};
    auto encodingWriter = impl::StringEncodingWriter{writer, encoding};
    if (encoding.writesBom(bomMode)) {
        encodingWriter.writeBom();
    }
    encodingWriter.write(normalizedCharacter(source));
    return writer.toByteBlock();
}

auto StringEncoderTraits<Char>::encodedLength(
    const Char &source, const StringEncoding encoding, const StringBomMode bomMode) -> unit::ByteLength {
    auto result = encoding.bomLength(bomMode);
    result.addOrThrow(normalizedCharacter(source).encodedBytes(encoding));
    return result;
}

auto StringEncoderTraits<Char>::encodeTo(
    const Char &source, mem::RingBuffer &buffer, const StringEncoding encoding, const StringBomMode bomMode)
    -> util::Result {
    const auto length = encodedLength(source, encoding, bomMode);
    if (isFailure(buffer.reserveAdditional(length))) {
        return util::Result::Failure;
    }
    auto ringWriter = mem::impl::RingBufferWriter{buffer};
    auto writer = impl::StringEncodingWriter{ringWriter, encoding};
    if (encoding.writesBom(bomMode)) {
        writer.writeBom();
    }
    writer.write(normalizedCharacter(source));
    ringWriter.commit();
    return util::Result::Success;
}

}
