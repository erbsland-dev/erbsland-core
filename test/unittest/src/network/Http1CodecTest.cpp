// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/debug/impl/StringDebugAccess.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/network/impl/http/codec/Http1ProtocolError.hpp>
#include <erbsland/network/impl/http/codec/Http1RequestDecoder.hpp>
#include <erbsland/network/impl/http/codec/Http1RequestEncoder.hpp>
#include <erbsland/network/impl/http/codec/Http1ResponseDecoder.hpp>
#include <erbsland/network/impl/http/codec/Http1ResponseEncoder.hpp>
#include <erbsland/text/impl/UnsafeU8StringAccess.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/TextHelper.hpp>

#include <span>
#include <string>
#include <string_view>

using namespace el::network;
using namespace el::text::literals;
using el::network::impl::Http1BodyFraming;
using el::network::impl::Http1CodecLimits;
using el::network::impl::Http1DecodeEvent;
using el::network::impl::Http1FailureReason;
using el::network::impl::Http1ProtocolError;
using el::network::impl::Http1RequestDecoder;
using el::network::impl::Http1RequestEncoder;
using el::network::impl::Http1ResponseDecoder;
using el::network::impl::Http1ResponseEncoder;

TESTED_TARGETS(
    Http1CodecLimits Http1DecodeEvent Http1FramingParser Http1ProtocolError Http1RequestDecoder Http1ResponseDecoder
        Http1RequestEncoder Http1ResponseEncoder)
class Http1CodecTest final : public el::UnitTest {
private:
    [[nodiscard]] static auto bytes(const std::string_view text) -> el::mem::ByteBlock {
        return el::mem::ByteBlock::fromSpan(std::span<const char>{text.data(), text.size()});
    }

    [[nodiscard]] static auto raw(const el::mem::ByteBlock &block) -> std::string {
        auto result = std::string{};
        result.reserve(block.length().toSizeT());
        for (const auto byte : block.span()) {
            result.push_back(byte.toChar());
        }
        return result;
    }

    template <typename Encoder>
    [[nodiscard]] static auto drain(Encoder &encoder) -> std::string {
        auto result = std::string{};
        while (!encoder.queuedOutputLength().isZero()) {
            result += raw(encoder.takeOutput(el::unit::ByteLength{7U}));
        }
        return result;
    }

    template <typename Function>
    void requireFailure(const Http1FailureReason reason, Function function) {
        auto thrown = false;
        try {
            function();
        } catch (const Http1ProtocolError &error) {
            thrown = true;
            REQUIRE_EQUAL(error.reason(), reason);
        }
        REQUIRE(thrown);
    }

public:
    void testFixedRequestEveryByteFragmentationAndPipeline() {
        const auto wire = std::string{"POST /one HTTP/1.1\r\nHost: example.test\r\nContent-Length: 5\r\n\r\nhello"
                                      "GET /two HTTP/1.0\r\n\r\n"};
        auto decoder = Http1RequestDecoder{};
        for (const auto character : wire) {
            REQUIRE(decoder.feed(bytes(std::string_view{&character, 1U}).span()).isAccepted());
        }
        const auto head = decoder.next();
        REQUIRE(head.has_value());
        REQUIRE_EQUAL(head->kind(), Http1DecodeEvent::Kind::RequestHead);
        REQUIRE_EQUAL(head->request().target(), "/one"_el);
        REQUIRE_EQUAL(head->framing(), Http1BodyFraming::FixedLength);
        REQUIRE_EQUAL(head->contentLength().value(), el::unit::ByteLength{5U});
        REQUIRE_EQUAL(raw(decoder.next()->data()), "hello");
        REQUIRE_EQUAL(decoder.next()->kind(), Http1DecodeEvent::Kind::Complete);
        decoder.reset();
        REQUIRE_EQUAL(decoder.next()->request().target(), "/two"_el);
        REQUIRE_EQUAL(decoder.next()->kind(), Http1DecodeEvent::Kind::Complete);
    }

    void testRawFieldsChunkingAndTrailers() {
        auto wire = std::string{"POST /raw HTTP/1.1\r\nTransfer-Encoding: chunked\r\nAuthorization:\t Bearer "};
        wire += el::unittest::th::stdStringFromHex("FF");
        wire += " \t\r\nCookie: a=";
        wire += el::unittest::th::stdStringFromHex("FE");
        wire += "\r\nX-Raw: in  terior\r\n\r\n4 \t; safe = token ; quoted = \"x\\\"y\"\r\nbody\r\n0\r\nX-Checksum: "
                "ok\r\n\r\n";
        auto decoder = Http1RequestDecoder{};
        REQUIRE(decoder.feed(bytes(wire).span()).isAccepted());
        const auto head = decoder.next();
        REQUIRE_EQUAL(head->framing(), Http1BodyFraming::Chunked);
        const auto authorization = head->request().headers().getFirst(HttpFieldType::Authorization);
        REQUIRE_FALSE(authorization.isValidUtf8());
        REQUIRE_EQUAL(authorization.length(), el::unit::ByteLength{8U});
        REQUIRE_FALSE(head->request().headers().getFirst(HttpFieldType::Cookie).isValidUtf8());
        REQUIRE_EQUAL(head->request().headers().getFirst("X-Raw"_el), "in  terior"_el);
        REQUIRE_EQUAL(raw(decoder.next()->data()), "body");
        const auto trailers = decoder.next();
        REQUIRE_EQUAL(trailers->kind(), Http1DecodeEvent::Kind::Trailers);
        REQUIRE_EQUAL(trailers->trailerFields().getFirst("X-Checksum"_el), "ok"_el);
        REQUIRE_EQUAL(decoder.next()->kind(), Http1DecodeEvent::Kind::Complete);
    }

    void testResponseModesAndOpaqueRemainder() {
        auto closeDecoder = Http1ResponseDecoder{};
        REQUIRE(closeDecoder.feed(bytes("HTTP/1.1 200 Fine\r\nX-Test: a\r\n\r\nclose body").span()).isAccepted());
        REQUIRE_EQUAL(closeDecoder.next()->framing(), Http1BodyFraming::CloseDelimited);
        REQUIRE_EQUAL(raw(closeDecoder.next()->data()), "close body");
        closeDecoder.endOfInput();
        REQUIRE_EQUAL(closeDecoder.next()->kind(), Http1DecodeEvent::Kind::Complete);

        auto headDecoder = Http1ResponseDecoder{HttpMethodType::Head};
        REQUIRE(headDecoder.feed(bytes("HTTP/1.1 200 OK\r\nContent-Length: 99\r\n\r\nnext").span()).isAccepted());
        REQUIRE_EQUAL(headDecoder.next()->framing(), Http1BodyFraming::None);
        REQUIRE_EQUAL(headDecoder.next()->kind(), Http1DecodeEvent::Kind::Complete);
        REQUIRE_EQUAL(headDecoder.retainedInputLength(), el::unit::ByteLength{4U});

        auto connectDecoder = Http1ResponseDecoder{HttpMethodType::Connect};
        auto connectWire = std::string{"HTTP/1.1 200 Connected\r\n\r\nopaque"};
        connectWire.push_back('\0');
        connectWire += "bytes";
        REQUIRE(connectDecoder.feed(bytes(connectWire).span()).isAccepted());
        REQUIRE_EQUAL(connectDecoder.next()->framing(), Http1BodyFraming::Opaque);
        REQUIRE_EQUAL(connectDecoder.next()->kind(), Http1DecodeEvent::Kind::Complete);
        REQUIRE_EQUAL(raw(connectDecoder.takeOpaqueRemainder()), connectWire.substr(connectWire.size() - 12U));

        auto informational = Http1ResponseDecoder{};
        REQUIRE(informational.feed(bytes("HTTP/1.1 103 Early Hints\r\nLink: </x>\r\n\r\n").span()).isAccepted());
        REQUIRE_EQUAL(informational.next()->response().status(), HttpStatus::EarlyHints);
        REQUIRE_EQUAL(informational.next()->kind(), Http1DecodeEvent::Kind::Complete);
    }

    void testEncoderDecoderRoundTripsAndRawReason() {
        auto requestHeaders = HttpHeaders{};
        requestHeaders.addField(HttpFieldType::TransferEncoding, "chunked"_el);
        auto requestEncoder = Http1RequestEncoder{
            HttpRequestHead{HttpMethodType::Post, "/upload"_el, HttpVersion::Http11, requestHeaders}};
        REQUIRE(requestEncoder.writeBody(bytes("abc").span()).isAccepted());
        auto trailers = HttpHeaders{};
        trailers.addField("X-Sum"_el, "123"_el);
        REQUIRE(requestEncoder.finish(trailers).isAccepted());
        const auto requestWire = drain(requestEncoder);
        REQUIRE_EQUAL(
            requestWire,
            "POST /upload HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nabc\r\n0\r\nX-Sum: 123\r\n\r\n");

        auto requestDecoder = Http1RequestDecoder{};
        REQUIRE(requestDecoder.feed(bytes(requestWire).span()).isAccepted());
        REQUIRE_EQUAL(requestDecoder.next()->request().target(), "/upload"_el);
        REQUIRE_EQUAL(raw(requestDecoder.next()->data()), "abc");
        REQUIRE_EQUAL(requestDecoder.next()->trailerFields(), trailers);

        auto responseHeaders = HttpHeaders{};
        responseHeaders.addField(HttpFieldType::ContentLength, "2"_el);
        responseHeaders.addField(
            HttpFieldType::SetCookie, el::text::String{el::unittest::th::stdStringFromHex("61 3D FF")});
        const auto malformedReason = el::text::String{el::unittest::th::stdStringFromHex("4F 4B FF")};
        auto responseEncoder = Http1ResponseEncoder{
            HttpResponseHead{HttpVersion::Http11, HttpStatus::Ok, malformedReason, responseHeaders}};
        REQUIRE(responseEncoder.writeBody(bytes("ok").span()).isAccepted());
        REQUIRE(responseEncoder.finish().isAccepted());
        const auto responseWire = drain(responseEncoder);
        REQUIRE_EQUAL(responseWire.substr(0U, 15U), std::string{"HTTP/1.1 200 OK"});
        REQUIRE_EQUAL(static_cast<unsigned char>(responseWire[15U]), 0xffU);

        auto responseDecoder = Http1ResponseDecoder{};
        REQUIRE(responseDecoder.feed(bytes(responseWire).span()).isAccepted());
        const auto decodedResponse = responseDecoder.next()->response();
        REQUIRE_FALSE(decodedResponse.reasonPhrase().isValidUtf8());
        REQUIRE_FALSE(decodedResponse.headers().getFirst(HttpFieldType::SetCookie).isValidUtf8());
        REQUIRE_EQUAL(raw(responseDecoder.next()->data()), "ok");
    }

    void testStrictFramingRegressions() {
        requireFailure(Http1FailureReason::AmbiguousFraming, [&] {
            auto decoder = Http1RequestDecoder{};
            static_cast<void>(
                decoder.feed(bytes("POST / HTTP/1.1\r\nContent-Length: 1\r\nContent-Length: 2\r\n\r\nx").span()));
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::AmbiguousFraming, [&] {
            auto decoder = Http1RequestDecoder{};
            static_cast<void>(decoder.feed(
                bytes("POST / HTTP/1.1\r\nContent-Length: 1\r\nTransfer-Encoding: chunked\r\n\r\n").span()));
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::AmbiguousFraming, [&] {
            auto decoder = Http1RequestDecoder{};
            static_cast<void>(decoder.feed(bytes("POST / HTTP/1.1\r\nTransfer-Encoding: chunked,\r\n\r\n").span()));
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::MalformedField, [&] {
            auto decoder = Http1RequestDecoder{};
            static_cast<void>(decoder.feed(bytes("GET / HTTP/1.1\r\nBad : value\r\n\r\n").span()));
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::MalformedField, [&] {
            auto decoder = Http1RequestDecoder{};
            static_cast<void>(decoder.feed(bytes("GET / HTTP/1.1\r\nFold: one\r\n two\r\n\r\n").span()));
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::MalformedChunk, [&] {
            auto decoder = Http1RequestDecoder{};
            static_cast<void>(decoder.feed(bytes("POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\nZ\r\n").span()));
            static_cast<void>(decoder.next());
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::ForbiddenTrailer, [&] {
            auto decoder = Http1RequestDecoder{};
            static_cast<void>(decoder.feed(
                bytes("POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n0\r\nContent-Length: 0\r\n\r\n").span()));
            static_cast<void>(decoder.next());
            static_cast<void>(decoder.next());
        });
    }

    void testStrictSyntaxAndChunkFailures() {
        requireFailure(Http1FailureReason::MalformedStartLine, [&] {
            auto decoder = Http1RequestDecoder{};
            static_cast<void>(decoder.feed(bytes("\r\nGET / HTTP/1.1\r\n\r\n").span()));
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::MalformedStartLine, [&] {
            auto decoder = Http1RequestDecoder{};
            static_cast<void>(decoder.feed(bytes("GET / HTTP/1.1\n").span()));
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::MalformedStartLine, [&] {
            auto decoder = Http1RequestDecoder{};
            static_cast<void>(decoder.feed(bytes("GET / HTTP/1.1\rX").span()));
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::UnsupportedVersion, [&] {
            auto decoder = Http1RequestDecoder{};
            static_cast<void>(decoder.feed(bytes("GET / HTTP/2.0\r\n\r\n").span()));
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::UnsupportedTransferCoding, [&] {
            auto decoder = Http1ResponseDecoder{};
            static_cast<void>(decoder.feed(bytes("HTTP/1.1 200 OK\r\nTransfer-Encoding: gzip\r\n\r\n").span()));
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::AmbiguousFraming, [&] {
            auto decoder = Http1RequestDecoder{};
            static_cast<void>(decoder.feed(bytes("POST / HTTP/1.1\r\nContent-Length: 1,,1\r\n\r\nx").span()));
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::MalformedChunk, [&] {
            auto decoder = Http1RequestDecoder{};
            static_cast<void>(
                decoder.feed(bytes("POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n1;=x\r\n").span()));
            static_cast<void>(decoder.next());
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::MalformedChunk, [&] {
            auto decoder = Http1RequestDecoder{};
            static_cast<void>(decoder.feed(
                bytes("POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n10000000000000000\r\n").span()));
            static_cast<void>(decoder.next());
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::ForbiddenTrailer, [&] {
            auto decoder = Http1ResponseDecoder{};
            static_cast<void>(decoder.feed(
                bytes("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n0\r\nSet-Cookie: a=b\r\n\r\n").span()));
            static_cast<void>(decoder.next());
            static_cast<void>(decoder.next());
        });
    }

    void testResponseEveryByteFragmentationAndInformationalPipeline() {
        const auto wire = std::string{"HTTP/1.1 200 Exact\r\nContent-Length: 5\r\n\r\nhello"};
        auto decoder = Http1ResponseDecoder{};
        auto body = std::string{};
        auto sawHead = false;
        auto sawComplete = false;
        for (const auto character : wire) {
            REQUIRE(decoder.feed(bytes(std::string_view{&character, 1U}).span()).isAccepted());
            while (const auto event = decoder.next()) {
                if (event->kind() == Http1DecodeEvent::Kind::ResponseHead) {
                    sawHead = true;
                    REQUIRE_EQUAL(event->response().reasonPhrase(), "Exact"_el);
                } else if (event->kind() == Http1DecodeEvent::Kind::Body) {
                    body += raw(event->data());
                } else if (event->kind() == Http1DecodeEvent::Kind::Complete) {
                    sawComplete = true;
                }
            }
        }
        REQUIRE(sawHead);
        REQUIRE(sawComplete);
        REQUIRE_EQUAL(body, "hello");

        auto informational = Http1ResponseDecoder{};
        REQUIRE(informational
                .feed(bytes("HTTP/1.1 103 Early Hints\r\n\r\nHTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n").span())
                .isAccepted());
        REQUIRE_EQUAL(informational.next()->response().status(), HttpStatus::EarlyHints);
        REQUIRE_EQUAL(informational.next()->kind(), Http1DecodeEvent::Kind::Complete);
        informational.reset();
        REQUIRE_EQUAL(informational.next()->response().status(), HttpStatus::Ok);
        REQUIRE_EQUAL(informational.next()->kind(), Http1DecodeEvent::Kind::Complete);
    }

    void testEofLimitsAndBackPressure() {
        requireFailure(Http1FailureReason::PrematureEndOfStream, [&] {
            auto decoder = Http1RequestDecoder{};
            static_cast<void>(decoder.feed(bytes("POST / HTTP/1.1\r\nContent-Length: 2\r\n\r\nx").span()));
            static_cast<void>(decoder.next());
            static_cast<void>(decoder.next());
            decoder.endOfInput();
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::StartLineTooLong, [&] {
            auto limits = Http1CodecLimits{}.setMaximumStartLineLength(el::unit::ByteLength{4U});
            auto decoder = Http1RequestDecoder{limits};
            static_cast<void>(decoder.feed(bytes("GET / HTTP/1.1\r\n").span()));
            static_cast<void>(decoder.next());
        });
        auto limited = Http1RequestDecoder{Http1CodecLimits{}.setMaximumInputLength(el::unit::ByteLength{4U})};
        REQUIRE(limited.feed(bytes("12345").span()).wouldBlock());
        REQUIRE(limited.retainedInputLength().isZero());

        auto headers = HttpHeaders{};
        headers.addField(HttpFieldType::ContentLength, "4"_el);
        auto encoder = Http1RequestEncoder{
            HttpRequestHead{HttpMethodType::Post, "/"_el, HttpVersion::Http11, headers},
            Http1CodecLimits{}.setMaximumOutputLength(el::unit::ByteLength{128U})};
        const auto before = encoder.queuedOutputLength();
        requireFailure(
            Http1FailureReason::InvalidState, [&] { static_cast<void>(encoder.writeBody(bytes("12345").span())); });
        REQUIRE_EQUAL(encoder.queuedOutputLength(), before);
    }

    void testInputAndOutputQueueWraparound() {
        auto firstInput = std::string{"GET / HTTP/1.1\r\nX-One: a\r\nX-Wrap: "};
        firstInput.append(29U, 'x');
        firstInput.push_back('\r');
        REQUIRE_EQUAL(firstInput.size(), std::size_t{64U});
        auto decoder = Http1RequestDecoder{Http1CodecLimits{}.setMaximumInputLength(el::unit::ByteLength{64U})};
        REQUIRE(decoder.feed(bytes(firstInput).span()).isAccepted());
        REQUIRE_FALSE(decoder.next().has_value());
        REQUIRE(decoder.feed(bytes("\n\r\n").span()).isAccepted());
        const auto head = decoder.next();
        REQUIRE(head.has_value());
        REQUIRE_EQUAL(head->request().headers().getFirst("X-One"_el), "a"_el);
        REQUIRE_EQUAL(head->request().headers().getFirst("X-Wrap"_el).length(), el::unit::ByteLength{29U});
        const auto firstField = head->request().headers().field(el::unit::ItemIndex::zero());
        REQUIRE_EQUAL(
            el::debug::impl::StringDebugAccess{firstField.name().text()}.backingStorageId(),
            el::debug::impl::StringDebugAccess{firstField.value()}.backingStorageId());
        REQUIRE_EQUAL(
            el::debug::impl::StringDebugAccess{head->request().method().text()}.backingStorageId(),
            el::debug::impl::StringDebugAccess{head->request().target()}.backingStorageId());
        REQUIRE_EQUAL(decoder.next()->kind(), Http1DecodeEvent::Kind::Complete);

        auto headers = HttpHeaders{};
        headers.addField(HttpFieldType::ContentLength, "40"_el);
        auto encoder = Http1RequestEncoder{
            HttpRequestHead{HttpMethodType::Post, "/"_el, HttpVersion::Http11, headers},
            Http1CodecLimits{}.setMaximumOutputLength(el::unit::ByteLength{64U})};
        auto wire = raw(encoder.takeOutput(el::unit::ByteLength{30U}));
        const auto body = std::string(40U, 'b');
        REQUIRE(encoder.writeBody(bytes(body).span()).isAccepted());
        REQUIRE(encoder.finish().isAccepted());
        wire += drain(encoder);
        REQUIRE_EQUAL(wire, std::string{"POST / HTTP/1.1\r\nContent-Length: 40\r\n\r\n"} + body);
    }

    void testRepeatedLengthsSpecialResponsesAndEveryLimit() {
        auto identical = Http1RequestDecoder{};
        REQUIRE(identical.feed(bytes("POST / HTTP/1.1\r\nContent-Length: 3, 3\r\nContent-Length: 3\r\n\r\nabc").span())
                .isAccepted());
        REQUIRE_EQUAL(identical.next()->contentLength().value(), el::unit::ByteLength{3U});
        REQUIRE_EQUAL(raw(identical.next()->data()), "abc");

        auto notModified = Http1ResponseDecoder{};
        REQUIRE(notModified.feed(bytes("HTTP/1.1 304 Not Modified\r\nContent-Length: 42\r\n\r\n").span()).isAccepted());
        REQUIRE_EQUAL(notModified.next()->framing(), Http1BodyFraming::None);
        auto switching = Http1ResponseDecoder{};
        REQUIRE(switching.feed(bytes("HTTP/1.1 101 Switching\r\nUpgrade: test\r\n\r\nopaque").span()).isAccepted());
        REQUIRE_EQUAL(switching.next()->framing(), Http1BodyFraming::Opaque);
        REQUIRE_EQUAL(switching.next()->kind(), Http1DecodeEvent::Kind::Complete);
        REQUIRE_EQUAL(raw(switching.takeOpaqueRemainder()), "opaque");

        requireFailure(Http1FailureReason::HeaderLimitExceeded, [&] {
            auto headerLimits = HttpHeaderLimits{}.setMaximumFieldCount(el::unit::ItemCount::zero());
            auto decoder = Http1RequestDecoder{Http1CodecLimits{}.setHeaderLimits(headerLimits)};
            static_cast<void>(decoder.feed(bytes("GET / HTTP/1.1\r\nX: y\r\n\r\n").span()));
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::HeaderLimitExceeded, [&] {
            auto headerLimits = HttpHeaderLimits{}.setMaximumNameLength(el::unit::ByteLength{1U});
            auto decoder = Http1RequestDecoder{Http1CodecLimits{}.setHeaderLimits(headerLimits)};
            static_cast<void>(decoder.feed(bytes("GET / HTTP/1.1\r\nXX: y\r\n\r\n").span()));
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::HeaderLimitExceeded, [&] {
            auto headerLimits = HttpHeaderLimits{}.setMaximumValueLength(el::unit::ByteLength{1U});
            auto decoder = Http1RequestDecoder{Http1CodecLimits{}.setHeaderLimits(headerLimits)};
            static_cast<void>(decoder.feed(bytes("GET / HTTP/1.1\r\nX: yy\r\n\r\n").span()));
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::HeaderLimitExceeded, [&] {
            auto headerLimits = HttpHeaderLimits{}.setMaximumAggregateLength(el::unit::ByteLength{4U});
            auto decoder = Http1RequestDecoder{Http1CodecLimits{}.setHeaderLimits(headerLimits)};
            static_cast<void>(decoder.feed(bytes("GET / HTTP/1.1\r\nX: y\r\n\r\n").span()));
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::ChunkMetadataTooLong, [&] {
            auto limits = Http1CodecLimits{}.setMaximumChunkMetadataLength(el::unit::ByteLength{1U});
            auto decoder = Http1RequestDecoder{limits};
            static_cast<void>(
                decoder.feed(bytes("POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n10\r\n").span()));
            static_cast<void>(decoder.next());
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::BodyLimitExceeded, [&] {
            auto limits = Http1CodecLimits{}.setMaximumBodyLength(el::unit::ByteLength{2U});
            auto decoder = Http1RequestDecoder{limits};
            static_cast<void>(decoder.feed(bytes("POST / HTTP/1.1\r\nContent-Length: 3\r\n\r\n").span()));
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::TrailerLimitExceeded, [&] {
            auto trailerLimits = HttpHeaderLimits{}.setMaximumFieldCount(el::unit::ItemCount::zero());
            auto decoder = Http1RequestDecoder{Http1CodecLimits{}.setTrailerLimits(trailerLimits)};
            static_cast<void>(
                decoder.feed(bytes("POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n0\r\nX: y\r\n\r\n").span()));
            static_cast<void>(decoder.next());
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::TrailerLimitExceeded, [&] {
            auto trailerLimits = HttpHeaderLimits{}.setMaximumNameLength(el::unit::ByteLength{1U});
            auto decoder = Http1RequestDecoder{Http1CodecLimits{}.setTrailerLimits(trailerLimits)};
            static_cast<void>(
                decoder.feed(bytes("POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n0\r\nXX: y\r\n\r\n").span()));
            static_cast<void>(decoder.next());
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::TrailerLimitExceeded, [&] {
            auto trailerLimits = HttpHeaderLimits{}.setMaximumValueLength(el::unit::ByteLength{1U});
            auto decoder = Http1RequestDecoder{Http1CodecLimits{}.setTrailerLimits(trailerLimits)};
            static_cast<void>(
                decoder.feed(bytes("POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n0\r\nX: yy\r\n\r\n").span()));
            static_cast<void>(decoder.next());
            static_cast<void>(decoder.next());
        });
        requireFailure(Http1FailureReason::TrailerLimitExceeded, [&] {
            auto trailerLimits = HttpHeaderLimits{}.setMaximumAggregateLength(el::unit::ByteLength{4U});
            auto decoder = Http1RequestDecoder{Http1CodecLimits{}.setTrailerLimits(trailerLimits)};
            static_cast<void>(
                decoder.feed(bytes("POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n0\r\nX: y\r\n\r\n").span()));
            static_cast<void>(decoder.next());
            static_cast<void>(decoder.next());
        });

        auto fixedHeaders = HttpHeaders{};
        fixedHeaders.addField(HttpFieldType::ContentLength, "6"_el);
        auto boundedEncoder = Http1RequestEncoder{
            HttpRequestHead{HttpMethodType::Post, "/"_el, HttpVersion::Http11, fixedHeaders},
            Http1CodecLimits{}.setMaximumOutputLength(el::unit::ByteLength{42U})};
        REQUIRE(boundedEncoder.writeBody(bytes("123456").span()).wouldBlock());
        static_cast<void>(boundedEncoder.takeOutput());
        REQUIRE(boundedEncoder.writeBody(bytes("123456").span()).isAccepted());

        auto incompleteEncoder =
            Http1RequestEncoder{HttpRequestHead{HttpMethodType::Post, "/"_el, HttpVersion::Http11, fixedHeaders}};
        const auto queuedBeforeFinish = incompleteEncoder.queuedOutputLength();
        requireFailure(Http1FailureReason::InvalidState, [&] { static_cast<void>(incompleteEncoder.finish()); });
        REQUIRE_EQUAL(incompleteEncoder.queuedOutputLength(), queuedBeforeFinish);
    }
};
