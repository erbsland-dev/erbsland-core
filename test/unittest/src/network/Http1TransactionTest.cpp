// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/LogicError.hpp>
#include <erbsland/event/EventLoop.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/network/impl/http/codec/Http1Transaction.hpp>
#include <erbsland/network/source/Connection.hpp>
#include <erbsland/network/source/ConnectionCloseContext.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <deque>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace el::event;
using namespace el::network;
using namespace el::network::impl;
using namespace el::text::literals;
namespace mem = el::mem;
namespace unit = el::unit;

TESTED_TARGETS(Http1Transaction Http1TransactionCallbacks Http1TransactionFailure Http1TransactionOptions)
class Http1TransactionTest final : public el::UnitTest {
    enum class ConnectionFlavor : std::uint8_t {
        Tcp,
        TlsClient,
        TlsServer,
    };

    struct Callbacks final {
        NetworkDataFn data;
        NetworkEventFn writable;
        ConnectionCloseFn closed;
        NetworkErrorFn error;
        NetworkEventFn final;
    };

    class MockConnection;

    class MockEditor final : public ConnectionEventEditor {
    public:
        MockEditor(el::event::EventSourcePtr source, el::event::EventsPtr target, Callbacks &callbacks) :
            ConnectionEventEditor{std::move(source), std::move(target)}, _callbacks{callbacks} {}

        auto onData(NetworkDataFn callback) -> MockEditor & override {
            _callbacks.data = std::move(callback);
            return *this;
        }
        auto onWritable(NetworkEventFn callback) -> MockEditor & override {
            _callbacks.writable = std::move(callback);
            return *this;
        }
        auto onClosed(ConnectionCloseFn callback) -> MockEditor & override {
            _callbacks.closed = std::move(callback);
            return *this;
        }
        auto onError(NetworkErrorFn callback) -> MockEditor & override {
            _callbacks.error = std::move(callback);
            return *this;
        }
        auto onFinal(NetworkEventFn callback) -> MockEditor & override {
            _callbacks.final = std::move(callback);
            return *this;
        }

    private:
        Callbacks &_callbacks;
    };

    class MockConnection final : public Connection {
    public:
        explicit MockConnection(EventsPtr owner, const ConnectionFlavor flavor = ConnectionFlavor::Tcp) :
            Connection{std::move(owner)}, _flavor{flavor} {}

        auto localEndpoint() const -> std::optional<IpEndpoint> override { return {}; }
        auto remoteEndpoint() const -> std::optional<IpEndpoint> override { return {}; }
        auto bufferLimits() const noexcept -> SocketBufferLimits override { return {}; }
        auto state() const noexcept -> ConnectionState override { return _state; }
        auto send(const mem::ByteBlock &data) -> NetworkSendStatus override {
            if (_aborted || _state != ConnectionState::Active) {
                return NetworkSendStatus::Closed;
            }
            if (_alwaysBlock) {
                return NetworkSendStatus::WouldBlock;
            }
            auto result = NetworkSendStatus::Accepted;
            if (!_sendResults.empty()) {
                result = _sendResults.front();
                _sendResults.pop_front();
            }
            if (result.isAccepted()) {
                _sent.push_back(data);
            }
            return result;
        }
        void pauseReceiving() override { _paused = true; }
        void resumeReceiving() override { _paused = false; }
        void close() override {
            _closeCalled = true;
            _state = ConnectionState::Closing;
        }
        void abort() noexcept override {
            _aborted = true;
            _state = ConnectionState::Failed;
        }
        auto events() -> MockEditor & override {
            auto target = currentOwnerEvents();
            if (!_editor) {
                _editor = std::make_unique<MockEditor>(shared_from_this(), std::move(target), _callbacks);
            }
            return *_editor;
        }

        void emitData(mem::ByteBlock data) {
            if (_callbacks.data) {
                _callbacks.data(std::move(data));
            }
        }
        void emitWritable() {
            if (_callbacks.writable) {
                _callbacks.writable();
            }
        }
        void emitError(NetworkErrorContext context) {
            _state = ConnectionState::Failed;
            if (_callbacks.error) {
                _callbacks.error(std::move(context));
            }
        }
        void emitClosed() {
            _state = ConnectionState::Closed;
            if (_callbacks.closed) {
                _callbacks.closed(ConnectionCloseContext{ConnectionCloseOrigin::Remote});
            }
        }
        void emitFinal() {
            _state = ConnectionState::Closed;
            if (_callbacks.final) {
                _callbacks.final();
            }
        }

        std::deque<NetworkSendStatus> _sendResults;
        std::vector<mem::ByteBlock> _sent;
        ConnectionState _state{ConnectionState::Active};
        bool _paused{};
        bool _closeCalled{};
        bool _aborted{};
        bool _alwaysBlock{};
        ConnectionFlavor _flavor;

    private:
        Callbacks _callbacks;
        std::unique_ptr<MockEditor> _editor;
    };

    struct Harness final {
        EventLoopPtr loop{EventLoop::create()};
        std::shared_ptr<MockConnection> connection{std::make_shared<MockConnection>(loop)};
    };

private:
    [[nodiscard]] static auto bytes(const std::string_view text) -> mem::ByteBlock {
        return mem::ByteBlock::fromSpan(std::span<const char>{text.data(), text.size()});
    }

    [[nodiscard]] static auto raw(const mem::ByteBlock &block) -> std::string {
        auto result = std::string{};
        for (const auto byte : block.span()) {
            result.push_back(byte.toChar());
        }
        return result;
    }

    [[nodiscard]] static auto sentWire(const MockConnection &connection) -> std::string {
        auto result = std::string{};
        for (const auto &block : connection._sent) {
            result += raw(block);
        }
        return result;
    }

    template <typename Function>
    void run(const EventLoopPtr &loop, Function function) {
        loop->invoke(std::move(function));
        REQUIRE(loop->runOnce());
    }

public:
    void testServerAggregationAndSequentialPipelineHandoff() {
        auto harness = Harness{};
        auto aggregate = std::string{};
        auto complete = false;
        auto final = false;
        auto transaction = Http1Transaction::createServer(harness.connection);
        run(harness.loop, [&]() -> void {
            transaction->callbacks().requestHead = [&](const Http1DecodeEvent &event) -> void {
                REQUIRE_EQUAL(event.request().target(), "/one"_el);
                REQUIRE(harness.connection->_paused);
                transaction->aggregateBody(unit::ByteLength{8U});
            };
            transaction->callbacks().aggregatedBody = [&](mem::ByteBlock data) -> void { aggregate = raw(data); };
            transaction->callbacks().complete = [&](const bool reusable) -> void {
                REQUIRE(reusable);
                complete = true;
            };
            transaction->callbacks().final = [&]() -> void { final = true; };
            transaction->start();
            harness.connection->emitData(bytes(
                "POST /one HTTP/1.1\r\nContent-Length: 5\r\n\r\nhello"
                "GET /two HTTP/1.1\r\n\r\n"));
            auto headers = HttpHeaders{};
            headers.addField(HttpFieldType::ContentLength, "2"_el);
            transaction->startResponse(HttpResponseHead{HttpVersion::Http11, HttpStatus::Ok, "OK"_el, headers});
            REQUIRE(transaction->sendBody(bytes("ok")).isAccepted());
            REQUIRE(transaction->finishBody().isAccepted());
            REQUIRE(complete);
            REQUIRE(final);
            REQUIRE_EQUAL(raw(transaction->takeBufferedInput()), "GET /two HTTP/1.1\r\n\r\n");
        });
        REQUIRE_EQUAL(aggregate, "hello");
        REQUIRE(sentWire(*harness.connection).find("HTTP/1.1 200 OK\r\n") != std::string::npos);
    }

    void testClientInformationalChunkedStreamingAndTrailers() {
        auto harness = Harness{};
        auto request = HttpRequestHead{HttpMethodType::Get, "/"_el, HttpVersion::Http11};
        auto transaction = Http1Transaction::createClient(harness.connection, request);
        auto informational = 0;
        auto body = std::string{};
        auto trailer = el::text::String{};
        auto completed = false;
        run(harness.loop, [&]() -> void {
            transaction->callbacks().informationalHead = [&](const Http1DecodeEvent &event) -> void {
                REQUIRE_EQUAL(event.response().status(), HttpStatus::EarlyHints);
                ++informational;
            };
            transaction->callbacks().responseHead = [&](const Http1DecodeEvent &) -> void {
                transaction->streamBody();
            };
            transaction->callbacks().bodyData = [&](mem::ByteBlock data) -> void { body += raw(data); };
            transaction->callbacks().trailers = [&](const HttpHeaders &headers) -> void {
                trailer = headers.getFirst("X-Sum"_el);
            };
            transaction->callbacks().complete = [&](const bool reusable) -> void { completed = reusable; };
            transaction->start();
            REQUIRE(transaction->finishBody().isAccepted());
            harness.connection->emitData(bytes(
                "HTTP/1.1 103 Early Hints\r\nLink: </a>\r\n\r\n"
                "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
                "2\r\nok\r\n0\r\nX-Sum: yes\r\n\r\n"));
        });
        REQUIRE_EQUAL(informational, 1);
        REQUIRE_EQUAL(body, "ok");
        REQUIRE_EQUAL(trailer, "yes"_el);
        REQUIRE(completed);
    }

    void testWireBackPressureRetainsOneBlockWithoutFalseWritable() {
        auto harness = Harness{};
        harness.connection->_sendResults.push_back(NetworkSendStatus::WouldBlock);
        auto transaction = Http1Transaction::createClient(
            harness.connection, HttpRequestHead{HttpMethodType::Get, "/blocked"_el, HttpVersion::Http11});
        auto writable = 0;
        run(harness.loop, [&]() -> void {
            transaction->callbacks().writable = [&]() -> void { ++writable; };
            transaction->start();
            const auto competing = Http1Transaction::createServer(harness.connection);
            REQUIRE_THROWS_AS(el::err::LogicError, competing->start());
            REQUIRE(harness.connection->_sent.empty());
            harness.connection->emitWritable();
            REQUIRE_EQUAL(writable, 0);
            REQUIRE(transaction->finishBody().isAccepted());
            transaction->cancel();
        });
        REQUIRE(sentWire(*harness.connection).find("GET /blocked HTTP/1.1\r\n") != std::string::npos);
    }

    void testSemanticBackPressureProducesOneWritableTransition() {
        auto harness = Harness{};
        harness.connection->_alwaysBlock = true;
        auto headers = HttpHeaders{};
        headers.addField(HttpFieldType::TransferEncoding, "chunked"_el);
        auto limits = Http1CodecLimits{};
        limits.setMaximumOutputLength(unit::ByteLength{80U});
        const auto options = Http1TransactionOptions{}.setCodecLimits(limits);
        auto transaction = Http1Transaction::createClient(
            harness.connection, HttpRequestHead{HttpMethodType::Post, "/"_el, HttpVersion::Http11, headers}, options);
        auto writable = 0;
        run(harness.loop, [&]() -> void {
            transaction->callbacks().writable = [&]() -> void { ++writable; };
            transaction->start();
            const auto body = mem::ByteBlock{unit::ByteLength{16U}, mem::Byte{0x61U}};
            while (transaction->sendBody(body).isAccepted()) {}
            REQUIRE_EQUAL(writable, 0);
            harness.connection->_alwaysBlock = false;
            harness.connection->emitWritable();
            REQUIRE_EQUAL(writable, 1);
            transaction->cancel();
        });
    }

    void testCloseDelimitedResponseCompletesOnlyAtEofAndForcesClosure() {
        auto harness = Harness{};
        auto transaction = Http1Transaction::createClient(
            harness.connection, HttpRequestHead{HttpMethodType::Get, "/close"_el, HttpVersion::Http11});
        auto body = std::string{};
        auto reusable = true;
        auto final = 0;
        run(harness.loop, [&]() -> void {
            transaction->callbacks().responseHead = [&](const Http1DecodeEvent &) -> void {
                transaction->streamBody();
            };
            transaction->callbacks().bodyData = [&](mem::ByteBlock data) -> void { body += raw(data); };
            transaction->callbacks().complete = [&](const bool value) -> void { reusable = value; };
            transaction->callbacks().final = [&]() -> void { ++final; };
            transaction->start();
            REQUIRE(transaction->finishBody().isAccepted());
            harness.connection->emitData(bytes("HTTP/1.1 200 OK\r\n\r\nuntil eof"));
            REQUIRE_EQUAL(final, 0);
            harness.connection->emitClosed();
            harness.connection->emitFinal();
        });
        REQUIRE_EQUAL(body, "until eof");
        REQUIRE_FALSE(reusable);
        REQUIRE_EQUAL(final, 1);
    }

    void testIdenticalEmptyTransactionAcrossCommonConnectionFlavors() {
        for (const auto flavor : {ConnectionFlavor::Tcp, ConnectionFlavor::TlsClient, ConnectionFlavor::TlsServer}) {
            const auto loop = EventLoop::create();
            const auto connection = std::make_shared<MockConnection>(loop, flavor);
            REQUIRE_EQUAL(connection->_flavor, flavor);
            const auto transaction = Http1Transaction::createServer(connection);
            auto completed = false;
            run(loop, [&]() -> void {
                transaction->callbacks().requestHead = [&](const Http1DecodeEvent &) -> void {
                    REQUIRE(transaction
                            ->sendInformationalResponse(
                                HttpResponseHead{HttpVersion::Http11, HttpStatus::EarlyHints, "Early Hints"_el})
                            .isAccepted());
                    auto headers = HttpHeaders{};
                    headers.addField(HttpFieldType::ContentLength, "0"_el);
                    transaction->startResponse(HttpResponseHead{HttpVersion::Http11, HttpStatus::Ok, "OK"_el, headers});
                    REQUIRE(transaction->finishBody().isAccepted());
                };
                transaction->callbacks().complete = [&](const bool reusable) -> void { completed = reusable; };
                transaction->start();
                connection->emitData(bytes("GET / HTTP/1.1\r\n\r\n"));
            });
            REQUIRE(completed);
            REQUIRE(sentWire(*connection).find("HTTP/1.1 103 Early Hints\r\n") != std::string::npos);
        }
    }

    void testKnownAggregateOverflowLeavesServerResponseAvailable() {
        auto harness = Harness{};
        auto transaction = Http1Transaction::createServer(harness.connection);
        auto failures = 0;
        run(harness.loop, [&]() -> void {
            transaction->callbacks().requestHead = [&](const Http1DecodeEvent &) -> void {
                transaction->aggregateBody(unit::ByteLength{3U});
            };
            transaction->callbacks().failure = [&](const Http1TransactionFailure &failure) -> void {
                REQUIRE_EQUAL(failure.kind(), Http1TransactionFailure::Kind::ResourceLimit);
                ++failures;
            };
            transaction->start();
            harness.connection->emitData(bytes("POST / HTTP/1.1\r\nContent-Length: 4\r\n\r\nbody"));
            REQUIRE_EQUAL(failures, 1);
            REQUIRE_FALSE(harness.connection->_aborted);
            auto headers = HttpHeaders{};
            headers.addField(HttpFieldType::ContentLength, "0"_el);
            transaction->startResponse(
                HttpResponseHead{HttpVersion::Http11, HttpStatus::ContentTooLarge, "Too Large"_el, headers});
            REQUIRE(transaction->finishBody().isAccepted());
            REQUIRE(harness.connection->_closeCalled);
            harness.connection->emitFinal();
        });
        REQUIRE_EQUAL(failures, 1);

        auto chunkedHarness = Harness{};
        auto chunkedFailures = 0;
        auto chunkedTransaction = Http1Transaction::createServer(chunkedHarness.connection);
        run(chunkedHarness.loop, [&]() -> void {
            chunkedTransaction->callbacks().requestHead = [&](const Http1DecodeEvent &) -> void {
                chunkedTransaction->aggregateBody(unit::ByteLength{3U});
            };
            chunkedTransaction->callbacks().failure = [&](const Http1TransactionFailure &failure) -> void {
                REQUIRE_EQUAL(failure.kind(), Http1TransactionFailure::Kind::ResourceLimit);
                REQUIRE_EQUAL(failure.protocolReason(), Http1FailureReason::BodyLimitExceeded);
                ++chunkedFailures;
            };
            chunkedTransaction->start();
            chunkedHarness.connection->emitData(
                bytes("POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n4\r\nbody\r\n0\r\n\r\n"));
            auto headers = HttpHeaders{};
            headers.addField(HttpFieldType::ContentLength, "0"_el);
            chunkedTransaction->startResponse(
                HttpResponseHead{HttpVersion::Http11, HttpStatus::ContentTooLarge, "Too Large"_el, headers});
            REQUIRE(chunkedTransaction->finishBody().isAccepted());
            chunkedHarness.connection->emitFinal();
        });
        REQUIRE_EQUAL(chunkedFailures, 1);
    }

    void testMalformedInputAndCallbackExceptionFinalizeExactlyOnce() {
        auto harness = Harness{};
        auto transaction = Http1Transaction::createServer(harness.connection);
        auto failures = 0;
        auto finals = 0;
        run(harness.loop, [&]() -> void {
            transaction->callbacks().requestHead = [&](const Http1DecodeEvent &) -> void {
                throw std::runtime_error{"application"};
            };
            transaction->callbacks().failure = [&](const Http1TransactionFailure &) -> void { ++failures; };
            transaction->callbacks().final = [&]() -> void { ++finals; };
            transaction->start();
            harness.connection->emitData(bytes("GET / HTTP/1.1\r\n\r\n"));
            harness.connection->emitFinal();
            harness.connection->emitData(bytes("GET /late HTTP/1.1\r\n\r\n"));
        });
        REQUIRE_EQUAL(failures, 1);
        REQUIRE_EQUAL(finals, 1);
        REQUIRE(harness.connection->_aborted);
    }

    void testStreamingPauseResumeAndExplicitRejection() {
        {
            auto harness = Harness{};
            auto body = std::string{};
            auto completed = false;
            auto transaction = Http1Transaction::createServer(harness.connection);
            run(harness.loop, [&]() -> void {
                transaction->callbacks().requestHead = [&](const Http1DecodeEvent &) -> void {
                    transaction->streamBody();
                    transaction->pauseBody();
                };
                transaction->callbacks().bodyData = [&](mem::ByteBlock data) -> void { body += raw(data); };
                transaction->callbacks().complete = [&](const bool reusable) -> void { completed = reusable; };
                transaction->start();
                harness.connection->emitData(bytes("POST / HTTP/1.1\r\nContent-Length: 4\r\n\r\nbody"));
                REQUIRE(harness.connection->_paused);
                REQUIRE(body.empty());
                transaction->resumeBody();
                REQUIRE_EQUAL(body, "body");
                auto headers = HttpHeaders{};
                headers.addField(HttpFieldType::ContentLength, "0"_el);
                transaction->startResponse(HttpResponseHead{HttpVersion::Http11, HttpStatus::Ok, "OK"_el, headers});
                REQUIRE(transaction->finishBody().isAccepted());
            });
            REQUIRE(completed);
        }
        {
            auto harness = Harness{};
            auto failures = 0;
            auto transaction = Http1Transaction::createServer(harness.connection);
            run(harness.loop, [&]() -> void {
                transaction->callbacks().requestHead = [&](const Http1DecodeEvent &) -> void {
                    transaction->rejectBody();
                };
                transaction->callbacks().failure = [&](const Http1TransactionFailure &failure) -> void {
                    REQUIRE_EQUAL(failure.kind(), Http1TransactionFailure::Kind::ResourceLimit);
                    ++failures;
                };
                transaction->start();
                harness.connection->emitData(bytes("POST / HTTP/1.1\r\nContent-Length: 4\r\n\r\nbody"));
                auto headers = HttpHeaders{};
                headers.addField(HttpFieldType::ContentLength, "0"_el);
                transaction->startResponse(
                    HttpResponseHead{HttpVersion::Http11, HttpStatus::ContentTooLarge, "Too Large"_el, headers});
                REQUIRE(transaction->finishBody().isAccepted());
                REQUIRE(harness.connection->_closeCalled);
                harness.connection->emitFinal();
            });
            REQUIRE_EQUAL(failures, 1);
        }
    }

    void testEarlyResponsesForceClosureAndSuppressBodies() {
        {
            auto harness = Harness{};
            auto bodyCallbacks = 0;
            auto reusable = true;
            auto transaction = Http1Transaction::createServer(harness.connection);
            run(harness.loop, [&]() -> void {
                transaction->callbacks().requestHead = [&](const Http1DecodeEvent &) -> void {
                    transaction->streamBody();
                    auto headers = HttpHeaders{};
                    headers.addField(HttpFieldType::ContentLength, "0"_el);
                    transaction->startResponse(
                        HttpResponseHead{HttpVersion::Http11, HttpStatus::BadRequest, "Bad Request"_el, headers});
                    REQUIRE(transaction->finishBody().isAccepted());
                };
                transaction->callbacks().bodyData = [&](mem::ByteBlock) -> void { ++bodyCallbacks; };
                transaction->callbacks().complete = [&](const bool value) -> void { reusable = value; };
                transaction->start();
                harness.connection->emitData(bytes("POST / HTTP/1.1\r\nContent-Length: 4\r\n\r\nbody"));
            });
            REQUIRE_EQUAL(bodyCallbacks, 0);
            REQUIRE_FALSE(reusable);
            REQUIRE(harness.connection->_closeCalled);
        }
        {
            auto harness = Harness{};
            auto headers = HttpHeaders{};
            headers.addField(HttpFieldType::TransferEncoding, "chunked"_el);
            auto transaction = Http1Transaction::createClient(
                harness.connection,
                HttpRequestHead{HttpMethodType::Post, "/"_el, HttpVersion::Http11, std::move(headers)});
            auto reusable = true;
            run(harness.loop, [&]() -> void {
                transaction->callbacks().complete = [&](const bool value) -> void { reusable = value; };
                transaction->start();
                REQUIRE(transaction->sendBody(bytes("upload")).isAccepted());
                harness.connection->emitData(bytes("HTTP/1.1 413 Too Large\r\nContent-Length: 0\r\n\r\n"));
                REQUIRE(transaction->sendBody(bytes("late")).isClosed());
            });
            REQUIRE_FALSE(reusable);
            REQUIRE(harness.connection->_closeCalled);
        }
    }

    void testProtocolTransportAndCancellationTerminalPaths() {
        {
            auto harness = Harness{};
            auto failures = 0;
            auto transaction = Http1Transaction::createServer(harness.connection);
            run(harness.loop, [&]() -> void {
                transaction->callbacks().failure = [&](const Http1TransactionFailure &failure) -> void {
                    REQUIRE_EQUAL(failure.kind(), Http1TransactionFailure::Kind::Protocol);
                    REQUIRE_EQUAL(failure.phase(), Http1TransactionFailure::Phase::Headers);
                    ++failures;
                };
                transaction->start();
                harness.connection->emitData(bytes("GET / HTTP/1.1\r\nBad Header\r\n\r\n"));
                REQUIRE_FALSE(harness.connection->_aborted);
                auto headers = HttpHeaders{};
                headers.addField(HttpFieldType::ContentLength, "0"_el);
                transaction->startResponse(
                    HttpResponseHead{HttpVersion::Http11, HttpStatus::BadRequest, "Bad Request"_el, headers});
                REQUIRE(transaction->finishBody().isAccepted());
                harness.connection->emitFinal();
            });
            REQUIRE_EQUAL(failures, 1);
        }
        {
            auto harness = Harness{};
            auto failures = 0;
            auto finals = 0;
            auto transaction = Http1Transaction::createClient(
                harness.connection, HttpRequestHead{HttpMethodType::Get, "/"_el, HttpVersion::Http11});
            run(harness.loop, [&]() -> void {
                transaction->callbacks().failure = [&](const Http1TransactionFailure &failure) -> void {
                    REQUIRE_EQUAL(failure.kind(), Http1TransactionFailure::Kind::Transport);
                    REQUIRE(failure.transportContext().has_value());
                    ++failures;
                };
                transaction->callbacks().final = [&]() -> void { ++finals; };
                transaction->start();
                harness.connection->emitError(NetworkErrorContext{"Lost"_el, "The mock connection failed."_el});
                harness.connection->emitFinal();
            });
            REQUIRE_EQUAL(failures, 1);
            REQUIRE_EQUAL(finals, 1);
            REQUIRE(harness.connection->_aborted);
        }
        {
            auto harness = Harness{};
            auto unsupported = false;
            auto transaction = Http1Transaction::createClient(
                harness.connection, HttpRequestHead{HttpMethodType::Get, "/"_el, HttpVersion::Http11});
            run(harness.loop, [&]() -> void {
                transaction->callbacks().failure = [&](const Http1TransactionFailure &failure) -> void {
                    unsupported = failure.kind() == Http1TransactionFailure::Kind::UnsupportedSwitch;
                };
                transaction->start();
                REQUIRE(transaction->finishBody().isAccepted());
                harness.connection->emitData(bytes("HTTP/1.1 101 Switching Protocols\r\n\r\nopaque"));
            });
            REQUIRE(unsupported);
            REQUIRE(harness.connection->_aborted);
        }
        {
            auto harness = Harness{};
            auto unsupported = false;
            auto transaction = Http1Transaction::createClient(
                harness.connection,
                HttpRequestHead{HttpMethodType::Connect, "example.test:443"_el, HttpVersion::Http11});
            run(harness.loop, [&]() -> void {
                transaction->callbacks().failure = [&](const Http1TransactionFailure &failure) -> void {
                    unsupported = failure.kind() == Http1TransactionFailure::Kind::UnsupportedSwitch;
                };
                transaction->start();
                REQUIRE(transaction->finishBody().isAccepted());
                harness.connection->emitData(bytes("HTTP/1.1 200 Connection Established\r\n\r\nopaque"));
            });
            REQUIRE(unsupported);
            REQUIRE(harness.connection->_aborted);
        }
        {
            auto harness = Harness{};
            auto finals = 0;
            auto transaction = Http1Transaction::createClient(
                harness.connection, HttpRequestHead{HttpMethodType::Get, "/"_el, HttpVersion::Http11});
            run(harness.loop, [&]() -> void {
                transaction->callbacks().final = [&]() -> void { ++finals; };
                transaction->start();
                transaction->cancel();
                transaction->cancel();
                harness.connection->emitFinal();
            });
            REQUIRE_EQUAL(finals, 1);
            REQUIRE(harness.connection->_aborted);
        }
    }

    void testTimeoutPhasesCloseEscalationAndStaleTimers() {
        {
            auto harness = Harness{};
            auto options = Http1TransactionOptions{};
            options.setHeaderTimeout(el::time::Milliseconds{2})
                .setTotalTimeout(el::time::Seconds{1})
                .setCloseTimeout(el::time::Milliseconds{2});
            auto failures = 0;
            auto finals = 0;
            auto transaction = Http1Transaction::createServer(harness.connection, options);
            run(harness.loop, [&]() -> void {
                transaction->callbacks().failure = [&](const Http1TransactionFailure &failure) -> void {
                    REQUIRE_EQUAL(failure.kind(), Http1TransactionFailure::Kind::Timeout);
                    REQUIRE_EQUAL(failure.phase(), Http1TransactionFailure::Phase::Headers);
                    ++failures;
                };
                transaction->callbacks().final = [&]() -> void { ++finals; };
                transaction->start();
            });
            REQUIRE(harness.loop->runOnce(el::time::Milliseconds{20}));
            run(harness.loop, [&]() -> void {
                auto headers = HttpHeaders{};
                headers.addField(HttpFieldType::ContentLength, "0"_el);
                transaction->startResponse(
                    HttpResponseHead{HttpVersion::Http11, HttpStatus::RequestTimeout, "Timeout"_el, headers});
                REQUIRE(transaction->finishBody().isAccepted());
            });
            REQUIRE(harness.loop->runOnce(el::time::Milliseconds{20}));
            REQUIRE_EQUAL(failures, 1);
            REQUIRE_EQUAL(finals, 1);
            REQUIRE(harness.connection->_aborted);
        }
        {
            auto harness = Harness{};
            auto options = Http1TransactionOptions{};
            options.setHeaderTimeout(el::time::Seconds{1})
                .setBodyIdleTimeout(el::time::Milliseconds{2})
                .setTotalTimeout(el::time::Seconds{1});
            auto bodyTimeout = false;
            auto transaction = Http1Transaction::createServer(harness.connection, options);
            run(harness.loop, [&]() -> void {
                transaction->callbacks().requestHead = [&](const Http1DecodeEvent &) -> void {
                    transaction->streamBody();
                };
                transaction->callbacks().failure = [&](const Http1TransactionFailure &failure) -> void {
                    bodyTimeout = failure.kind() == Http1TransactionFailure::Kind::Timeout &&
                        failure.phase() == Http1TransactionFailure::Phase::Body;
                };
                transaction->start();
                harness.connection->emitData(bytes("POST / HTTP/1.1\r\nContent-Length: 4\r\n\r\na"));
            });
            for (auto attempt = 0; attempt < 3 && !bodyTimeout; ++attempt) {
                static_cast<void>(harness.loop->runOnce(el::time::Milliseconds{20}));
            }
            REQUIRE(bodyTimeout);
            transaction->cancel();
        }
        {
            auto harness = Harness{};
            auto options = Http1TransactionOptions{};
            options.setHeaderTimeout(el::time::Seconds{1}).setTotalTimeout(el::time::Milliseconds{2});
            auto totalTimeout = false;
            auto transaction = Http1Transaction::createServer(harness.connection, options);
            run(harness.loop, [&]() -> void {
                transaction->callbacks().failure = [&](const Http1TransactionFailure &failure) -> void {
                    totalTimeout = failure.kind() == Http1TransactionFailure::Kind::Timeout &&
                        failure.phase() == Http1TransactionFailure::Phase::Total;
                };
                transaction->start();
            });
            REQUIRE(harness.loop->runOnce(el::time::Milliseconds{20}));
            REQUIRE(totalTimeout);
            REQUIRE(harness.connection->_aborted);
        }
        {
            auto harness = Harness{};
            auto options = Http1TransactionOptions{};
            options.setHeaderTimeout(el::time::Milliseconds{2}).setTotalTimeout(el::time::Milliseconds{3});
            auto first = Http1Transaction::createServer(harness.connection, options);
            auto second = Http1TransactionPtr{};
            auto staleFailure = false;
            run(harness.loop, [&]() -> void {
                first->callbacks().requestHead = [&](const Http1DecodeEvent &) -> void {
                    auto headers = HttpHeaders{};
                    headers.addField(HttpFieldType::ContentLength, "0"_el);
                    first->startResponse(HttpResponseHead{HttpVersion::Http11, HttpStatus::Ok, "OK"_el, headers});
                    REQUIRE(first->finishBody().isAccepted());
                };
                first->start();
                harness.connection->emitData(bytes("GET /first HTTP/1.1\r\n\r\n"));
                second = Http1Transaction::createServer(harness.connection);
                second->callbacks().failure = [&](const Http1TransactionFailure &) -> void { staleFailure = true; };
                second->start();
            });
            REQUIRE(harness.loop->runOnce(el::time::Milliseconds{20}));
            REQUIRE(harness.loop->runOnce(el::time::Milliseconds{20}));
            REQUIRE_FALSE(staleFailure);
            REQUIRE_EQUAL(second->state(), Http1Transaction::State::ReceivingHeaders);
            run(harness.loop, [&]() -> void { second->cancel(); });
        }
    }
};
