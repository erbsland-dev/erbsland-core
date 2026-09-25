// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PlaceholderDemos.hpp"

#include <erbsland/text/placeholder/Filter.hpp>
#include <erbsland/text/placeholder/Replacer.hpp>
#include <erbsland/text/placeholder/ReplacerError.hpp>
#include <erbsland/text/StringFormat.hpp>

#include <memory>

namespace demo {

/// Add a destination-specific label to a placeholder value.
///
/// A filter receives the source value or the result of the previous filter. Its parameter selects the label;
/// `validate()` checks that choice without needing a value to transform.
class GalleryLabelFilter final : public el::placeholder::Filter {
public:
    [[nodiscard]] auto filterNames() const -> el::StringList override { return el::StringList{"gallery_label"_el}; }

    [[nodiscard]] auto apply(const el::String &, const el::String &parameter, const el::String &value)
        -> el::String override {
        if (parameter != "room"_el) {
            throw el::placeholder::ReplacerError{
                el::placeholder::ReplacerErrorCategory::Syntax, "Expected gallery label 'room'."_el};
        }
        return el::StringFormat{"Room: {}"_el}.build(value);
    }

    [[nodiscard]] auto validate(const el::String &, const el::String &parameter) -> bool override {
        return parameter == "room"_el;
    }
};

/// Register and chain a custom filter after a built-in text filter.
void useCustomFilter() {
    auto replacer = el::placeholder::Replacer{};
    replacer.setVariableSource({{{"movement"_el, u8"  Φουτουρισμός  "_el}}});
    replacer.addTextFilters();
    replacer.addFilter(std::make_shared<GalleryLabelFilter>());

    el::io::printLine(replacer.replaceOrThrow("${var:movement|trim|gallery_label:room}"_el));
}

}
