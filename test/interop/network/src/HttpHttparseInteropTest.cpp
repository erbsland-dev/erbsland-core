// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "InteropEnvironment.hpp"

#include <erbsland/network/http/HttpRequestHead.hpp>
#include <erbsland/network/impl/http/codec/Http1RequestDecoder.hpp>
#include <erbsland/network/impl/http/codec/Http1ResponseDecoder.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <span>
#include <string>
#include <string_view>

using namespace el::network;
using el::network::impl::Http1RequestDecoder;
using el::network::impl::Http1ResponseDecoder;

TESTED_TARGETS(Http1RequestDecoder Http1ResponseDecoder)
class HttpHttparseInteropTest final : public el::UnitTest {
private:
    [[nodiscard]] static auto hex(const std::string_view bytes) -> std::string {
        constexpr auto digits = std::string_view{"0123456789abcdef"};
        auto result = std::string{};
        result.reserve(bytes.size() * 2U);
        for (const auto value : bytes) {
            const auto byte = static_cast<unsigned char>(value);
            result.push_back(digits[byte >> 4U]);
            result.push_back(digits[byte & 0x0fU]);
        }
        return result;
    }

    [[nodiscard]] static auto span(const std::string_view bytes) -> el::mem::ConstByteSpan {
        return el::mem::toConstByteSpan(std::span<const char>{bytes.data(), bytes.size()});
    }

public:
    void testRequestStatusAndHeaderAgreement() {
        const auto requestWire = std::string{"GET /a?b=1 HTTP/1.1\r\nHost: example.test\r\nX-Raw: a"} +
            el::unittest::th::stdStringFromHex("FF") + "b\r\n\r\n";
        auto requestDecoder = Http1RequestDecoder{};
        REQUIRE(requestDecoder.feed(span(requestWire)).isAccepted());
        const auto request = requestDecoder.next();
        REQUIRE(request.has_value());
        REQUIRE_EQUAL(request->request().target(), el::text::String{"/a?b=1"});
        const auto requestReference = InteropEnvironment::instance().compareHttp("request", hex(requestWire));
        REQUIRE_EQUAL(requestReference.accepted, std::optional<std::uint64_t>{1U});
        REQUIRE_EQUAL(
            requestReference.parsed, "55:1:474554:2f613f623d31:486f7374=6578616d706c652e74657374:582d526177=61ff62");

        const auto responseWire = std::string{"HTTP/1.1 204 No Content\r\nX-Value: exact\r\n\r\n"};
        auto responseDecoder = Http1ResponseDecoder{};
        REQUIRE(responseDecoder.feed(span(responseWire)).isAccepted());
        const auto response = responseDecoder.next();
        REQUIRE(response.has_value());
        REQUIRE_EQUAL(response->response().status(), HttpStatus::NoContent);
        const auto responseReference = InteropEnvironment::instance().compareHttp("response", hex(responseWire));
        REQUIRE_EQUAL(responseReference.accepted, std::optional<std::uint64_t>{1U});
        REQUIRE_EQUAL(responseReference.parsed, "43:1:204:4e6f20436f6e74656e74:582d56616c7565=6578616374");
    }

    void testMalformedAndChunkSizeAgreement() {
        const auto malformed = std::string{"GET / HTTP/1.1\r\nBad : x\r\n\r\n"};
        const auto requestReference = InteropEnvironment::instance().compareHttp("request", hex(malformed));
        REQUIRE_EQUAL(requestReference.accepted, std::optional<std::uint64_t>{0U});

        const auto validChunk = InteropEnvironment::instance().compareHttp("chunk-size", hex("7f;ok=yes\r\n"));
        REQUIRE_EQUAL(validChunk.accepted, std::optional<std::uint64_t>{1U});
        REQUIRE_EQUAL(validChunk.parsed, "11:127");
        const auto badChunk = InteropEnvironment::instance().compareHttp("chunk-size", hex("xyz\r\n"));
        REQUIRE_EQUAL(badChunk.accepted, std::optional<std::uint64_t>{0U});
    }
};
