// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/mem/SharedDataPointer.hpp>
#include <erbsland/mem/SharedVirtualData.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace demo {

/// Preserve a polymorphic backend while detaching a copy-on-write value.
///
/// Every backend derives from `SharedVirtualData` and implements `clone()`.
/// `SharedDataPointer` calls that virtual function on the first write to a
/// shared value, so the detached object keeps its original dynamic type.
class DesignSequence {
    class Data : public el::mem::SharedVirtualData {
    public:
        [[nodiscard]] virtual auto kind() const noexcept -> el::String = 0;
        [[nodiscard]] virtual auto first() const noexcept -> std::uint32_t = 0;
        virtual void setFirst(std::uint32_t value) = 0;
    };

    class ByteData final : public Data {
    public:
        explicit ByteData(std::vector<std::uint8_t> values) : _values{std::move(values)} {}
        [[nodiscard]] auto clone() const -> ByteData * override { return new ByteData{*this}; }
        [[nodiscard]] auto kind() const noexcept -> el::String override { return "bytes"_el; }
        [[nodiscard]] auto first() const noexcept -> std::uint32_t override { return _values.front(); }
        void setFirst(const std::uint32_t value) override { _values.front() = static_cast<std::uint8_t>(value); }

    private:
        std::vector<std::uint8_t> _values;
    };

    class IntegerData final : public Data {
    public:
        explicit IntegerData(std::vector<std::uint32_t> values) : _values{std::move(values)} {}
        [[nodiscard]] auto clone() const -> IntegerData * override { return new IntegerData{*this}; }
        [[nodiscard]] auto kind() const noexcept -> el::String override { return "integers"_el; }
        [[nodiscard]] auto first() const noexcept -> std::uint32_t override { return _values.front(); }
        void setFirst(const std::uint32_t value) override { _values.front() = value; }

    private:
        std::vector<std::uint32_t> _values;
    };

public:
    [[nodiscard]] static auto fromBytes(std::vector<std::uint8_t> values) -> DesignSequence {
        return DesignSequence{new ByteData{std::move(values)}};
    }
    [[nodiscard]] static auto fromIntegers(std::vector<std::uint32_t> values) -> DesignSequence {
        return DesignSequence{new IntegerData{std::move(values)}};
    }

    void setFirst(const std::uint32_t value) { _data->setFirst(value); }
    [[nodiscard]] auto kind() const noexcept -> el::String { return _data->kind(); }
    [[nodiscard]] auto first() const noexcept -> std::uint32_t { return _data->first(); }

private:
    explicit DesignSequence(Data *data) : _data{data} {}

private:
    el::mem::SharedDataPointer<Data> _data;
};

void polymorphicCopyOnWrite() {
    const auto byteSequence = DesignSequence::fromBytes({12U, 24U, 36U});
    auto variation = byteSequence;

    // Virtual cloning keeps the byte backend when the variation detaches.
    variation.setFirst(48U);

    const auto integerSequence = DesignSequence::fromIntegers({1000U, 2000U});
    el::io::printLine("Byte backend      : "_el, variation.kind());
    el::io::printLine("Original first    : "_el, byteSequence.first());
    el::io::printLine("Variation first   : "_el, variation.first());
    el::io::printLine("Integer backend   : "_el, integerSequence.kind());
}

}
