// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringEncoder.hpp"

#include "impl/StringConversionTools.hpp"

namespace erbsland::text {

template <impl::AnyStringOrStringEditorType T>
auto StringEncoder<T>::encode(const StringEncoding encoding, const StringBomMode bomMode) const -> mem::ByteBlock {
    return impl::StringConversionTools::encode(*_source, encoding, bomMode);
}

template <impl::AnyStringOrStringEditorType T>
auto StringEncoder<T>::encodedLength(const StringEncoding encoding, const StringBomMode bomMode) const
    -> unit::ByteLength {
    return impl::StringConversionTools::encodedLength(*_source, encoding, bomMode);
}

template <impl::AnyStringOrStringEditorType T>
auto StringEncoder<T>::encodeTo(
    mem::RingBuffer &buffer, const StringEncoding encoding, const StringBomMode bomMode) const -> util::Result {
    return impl::StringConversionTools::encodeTo(*_source, buffer, encoding, bomMode);
}

template class StringEncoder<U8StringEditor>;
template class StringEncoder<U8String>;
template class StringEncoder<U16StringEditor>;
template class StringEncoder<U16String>;
template class StringEncoder<U32StringEditor>;
template class StringEncoder<U32String>;

}
