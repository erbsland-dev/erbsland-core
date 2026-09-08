// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/mem/SharedData.hpp>
#include <erbsland/mem/SharedDataPointer.hpp>

#include <utility>
#include <vector>

namespace demo {

/// Build a value type with an intrusive copy-on-write data object.
///
/// The data derives from `SharedData`, while the public value stores a
/// `SharedDataPointer`. Copies initially share their data. Mutable pointer
/// access detaches automatically and invokes the data type's copy constructor.
class DesignBoard {
    class Data final : public el::mem::SharedData {
    public:
        Data(el::String title, std::vector<el::String> concepts) :
            _title{std::move(title)}, _concepts{std::move(concepts)} {}
        Data(const Data &other) : SharedData{other}, _title{other._title}, _concepts{other._concepts} {}

        el::String _title;
        std::vector<el::String> _concepts;
    };

public:
    DesignBoard(el::String title, std::vector<el::String> concepts) :
        _data{new Data{std::move(title), std::move(concepts)}} {}

    void addConcept(el::String conceptName) { _data->_concepts.emplace_back(std::move(conceptName)); }
    [[nodiscard]] auto title() const noexcept -> const el::String & { return _data->_title; }
    [[nodiscard]] auto conceptCount() const noexcept -> std::size_t { return _data->_concepts.size(); }
    [[nodiscard]] auto isShared() const noexcept -> bool { return _data.isShared(); }

private:
    el::mem::SharedDataPointer<Data> _data;
};

void regularCopyOnWrite() {
    auto original = DesignBoard{"Rolig form"_el, {"cirkel"_el, "bølge"_el}};

    // Copying the value only shares its data object.
    auto variation = original;
    const auto sharedBeforeWrite = original.isShared() && variation.isShared();

    // The first mutable access copies the shared data before changing it.
    variation.addConcept("lys"_el);

    el::io::printLine("Board             : "_el, original.title());
    el::io::printLine("Shared after copy : "_el, el::BooleanFormat::yesNo(), sharedBeforeWrite);
    el::io::printLine("Original concepts : "_el, original.conceptCount());
    el::io::printLine("Variation concepts: "_el, variation.conceptCount());
}

}
