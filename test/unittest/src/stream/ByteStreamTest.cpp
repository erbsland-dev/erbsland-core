// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/StreamError.hpp>
#include <erbsland/stream/ByteInputStream.hpp>
#include <erbsland/stream/ByteOutputStream.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <vector>

using el::err::StreamError;
using el::mem::Byte;
using el::mem::Endianness;
using el::stream::ByteInputStream;
using el::stream::ByteOutputStream;
using el::unit::ByteLength;

TESTED_TARGETS(ByteInputStream ByteOutputStream)
class ByteStreamTest final : public el::UnitTest {
    class MemoryInputStream final : public ByteInputStream {
    public:
        explicit MemoryInputStream(std::vector<uint8_t> bytes) {
            _bytes.reserve(bytes.size());
            for (const auto byte : bytes) {
                _bytes.push_back(Byte{byte});
            }
        }

    public: // implement ByteInputStream
        [[nodiscard]] auto isOpen() const noexcept -> bool override { return _open; }

        void close() override { _open = false; }

        [[nodiscard]] auto read(std::span<Byte> destination) -> ByteLength override {
            if (!_open) {
                throw StreamError{"Closed."};
            }
            const auto available = _bytes.size() - _position;
            const auto count = std::min(destination.size(), available);
            std::copy_n(_bytes.data() + _position, count, destination.data());
            _position += count;
            return ByteLength::fromSizeT(count);
        }

    public:
        using ByteInputStream::read;

    private:
        std::vector<Byte> _bytes;
        std::size_t _position{0};
        bool _open{true};
    };

    class MemoryOutputStream final : public ByteOutputStream {
    public: // implement ByteOutputStream
        [[nodiscard]] auto isOpen() const noexcept -> bool override { return _open; }

        void flush() override { flushCount += 1U; }

        void close() override { _open = false; }

        void write(std::span<const Byte> bytes) override {
            if (!_open) {
                throw StreamError{"Closed."};
            }
            for (const auto byte : bytes) {
                data.push_back(byte.toUInt8());
            }
        }

    public:
        using ByteOutputStream::write;

    public:
        std::vector<uint8_t> data;
        std::size_t flushCount{0};

    private:
        bool _open{true};
    };

public:
    void testInputConvenienceMethods() {
        auto stream = MemoryInputStream{{0x34U, 0x12U, 0xabU, 0xcdU, 0xffU}};

        const auto byte = stream.readByte();
        REQUIRE(byte.has_value());
        REQUIRE_EQUAL(*byte, Byte{0x34U});
        REQUIRE_EQUAL(stream.readUInt8OrThrow(), uint8_t{0x12U});

        stream.setEndianness(Endianness::Big);
        REQUIRE_EQUAL(stream.readUInt16OrThrow(), uint16_t{0xabcdU});
        REQUIRE_EQUAL(stream.readInt8OrThrow(), int8_t{-1});
        REQUIRE_FALSE(stream.readByte().has_value());
        REQUIRE_THROWS_AS(StreamError, stream.readByteOrThrow());
    }

    void testInputExactReads() {
        auto stream = MemoryInputStream{{1U, 2U, 3U}};

        const auto first = stream.readExact(ByteLength{2U});
        REQUIRE(first.has_value());
        REQUIRE_EQUAL(first->toUInt8Vector(), std::vector<uint8_t>({1U, 2U}));

        const auto second = stream.readExact(ByteLength{2U});
        REQUIRE_FALSE(second.has_value());
        REQUIRE_FALSE(stream.readByte().has_value());
    }

    void testInputIntegerPartialConsumesBytes() {
        auto stream = MemoryInputStream{{0x34U}};

        REQUIRE_FALSE(stream.readUInt16().has_value());
        REQUIRE_FALSE(stream.readByte().has_value());
        REQUIRE_THROWS_AS(StreamError, stream.readUInt16OrThrow());
    }

    void testReadAll() {
        auto stream = MemoryInputStream{{1U, 2U, 3U}};

        REQUIRE_EQUAL(stream.readAll().toUInt8Vector(), std::vector<uint8_t>({1U, 2U, 3U}));
        REQUIRE(stream.readAll().isEmpty());
    }

    void testOutputConvenienceMethods() {
        auto stream = MemoryOutputStream{};

        stream.write(Byte{1U});
        stream.writeUInt16(0x1234U);
        stream.setEndianness(Endianness::Big);
        stream.writeUInt16(0xabcdU);
        stream.writeInt8(-1);
        stream.flush();

        REQUIRE_EQUAL(stream.data, std::vector<uint8_t>({1U, 0x34U, 0x12U, 0xabU, 0xcdU, 0xffU}));
        REQUIRE_EQUAL(stream.flushCount, std::size_t{1U});
    }
};
