// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/Exception.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/punycode/PunycodeDecoder.hpp>
#include <erbsland/text/punycode/PunycodeEncoder.hpp>
#include <erbsland/text/punycode/PunycodeOptions.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

class PunycodeFuzzInput final {
public:
    explicit PunycodeFuzzInput(const std::span<const uint8_t> data) : _data{data} {}

    [[nodiscard]] auto run() const -> int {
        if (_data.empty() || _data.size() > maximumInputSize) {
            return 0;
        }
        const auto operation = _data.front() % 4U;
        const auto bytes = std::string_view{
            reinterpret_cast<const char *>(_data.data() + 1U), _data.size() - 1U};
        const auto text = erbsland::text::String{bytes};
        try {
            switch (operation) {
            case 0U:
                static_cast<void>(erbsland::text::punycode::PunycodeDecoder{text}.decodeOrThrow());
                break;
            case 1U:
                static_cast<void>(erbsland::text::punycode::PunycodeEncoder{text}.encodeOrThrow());
                break;
            case 2U:
                static_cast<void>(erbsland::text::punycode::PunycodeDecoder{
                    text, erbsland::text::punycode::PunycodeOptions::network()}.decodeOrThrow());
                break;
            default:
                static_cast<void>(erbsland::text::punycode::PunycodeEncoder{
                    text, erbsland::text::punycode::PunycodeOptions::network()}.encodeOrThrow());
                break;
            }
        } catch (const erbsland::err::Exception &) {
            // Malformed encodings, contextual failures, bidi failures, and limits are expected parse outcomes.
        }
        return 0;
    }

private:
    static constexpr auto maximumInputSize = std::size_t{4096U};
    std::span<const uint8_t> _data;
};

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t *data, const std::size_t size) -> int {
    return PunycodeFuzzInput{std::span<const uint8_t>{data, size}}.run();
}
