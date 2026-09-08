// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/mem/SharedArrayData.hpp>
#include <erbsland/mem/SharedDataPointer.hpp>

#include <cstdint>

namespace demo {

/// Store a copy-on-write byte array in one allocation.
///
/// `SharedArrayData` combines its sharing metadata and trailing element array.
/// It creates, clones, and destroys that allocation through the matching
/// `SharedDataPointer` traits.
class ToneStrip {
    using Data = el::mem::SharedArrayData<std::uint8_t>;

public:
    explicit ToneStrip(const std::uint32_t size) : _data{Data::create(size, size)} {}

    void set(const std::uint32_t index, const std::uint8_t value) { _data->data()[index] = value; }
    [[nodiscard]] auto at(const std::uint32_t index) const noexcept -> std::uint8_t { return _data->data()[index]; }
    [[nodiscard]] auto size() const noexcept -> std::uint32_t { return _data->size(); }
    [[nodiscard]] auto isShared() const noexcept -> bool { return _data.isShared(); }

private:
    el::mem::SharedDataPointer<Data> _data;
};

void arrayCopyOnWrite() {
    auto palette = ToneStrip{3U};
    palette.set(0U, 18U);
    palette.set(1U, 90U);
    palette.set(2U, 160U);

    // Copies share the header and array until mutable access detaches them.
    auto brighter = palette;
    const auto sharedBeforeWrite = palette.isShared();
    brighter.set(0U, 64U);

    el::io::printLine("Concept           : nordisk lys"_el);
    el::io::printLine("Tone count        : "_el, palette.size());
    el::io::printLine("Shared after copy : "_el, el::BooleanFormat::yesNo(), sharedBeforeWrite);
    el::io::printLine("Original first    : "_el, palette.at(0U));
    el::io::printLine("Brighter first    : "_el, brighter.at(0U));
}

}
