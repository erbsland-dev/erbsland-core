// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PlaceholderDemos.hpp"

#include <erbsland/text/placeholder/Replacer.hpp>
#include <erbsland/text/placeholder/ReplacerError.hpp>
#include <erbsland/text/placeholder/Source.hpp>
#include <erbsland/text/StringFormat.hpp>

#include <memory>

namespace demo {

/// Provide exhibition text from an application-owned catalog.
///
/// A source publishes its accepted names and resolves a case-preserving parameter. A failed lookup throws
/// `ReplacerError`; overriding `validate()` lets syntax checks avoid querying the catalog.
class GallerySource final : public el::placeholder::Source {
public:
    [[nodiscard]] auto sourceNames() const -> el::StringList override { return el::StringList{"gallery"_el}; }

    [[nodiscard]] auto resolve(const el::String &, const el::String &parameter) -> el::String override {
        if (parameter == "movement"_el) {
            return u8"Συμβολισμός"_el;
        }
        throw el::placeholder::ReplacerError{
            el::placeholder::ReplacerErrorCategory::ValueNotFound,
            el::StringFormat{"Unknown gallery field: {}"_el}.build(parameter)};
    }

    [[nodiscard]] auto validate(const el::String &, const el::String &parameter) -> bool override {
        return parameter == "movement"_el;
    }
};

/// Register a custom source before expanding a gallery caption.
void useCustomSource() {
    auto replacer = el::placeholder::Replacer{};
    const auto source = std::make_shared<GallerySource>();
    replacer.addSource(source);

    el::io::printLine(replacer.replaceOrThrow(u8"Featured movement: ${gallery:movement}"_el));
    el::io::printLine("Valid caption: "_el, replacer.validate("${gallery:movement}"_el));
}

}
