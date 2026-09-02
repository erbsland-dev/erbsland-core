// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RemappedBufferTestSupport.hpp"

#include <erbsland/block/StdFormat.hpp>
#include <erbsland/cterm/RemappedBuffer.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <array>
#include <format>
#include <random>
#include <string>
#include <vector>

TESTED_TARGETS(RemappedBuffer)
class RemappedBufferTest final : public UNITTEST_SUBCLASS(RemappedBufferTestSupport) {
public:
    void testConstructorCloneAndWideCharacters() {
        forEachOrientation([&](const geometry::Orientation orientation) -> void {
            const auto fillChar = Block{U'X', fg::Yellow, bg::Blue};
            auto buffer = RemappedBuffer{block::Size{3, 2}, orientation, fillChar};

            REQUIRE_EQUAL(buffer.size(), block::Size(3, 2));
            buffer.size().forEach([&](const block::Position pos) -> void {
                REQUIRE_EQUAL(buffer.get(pos), U'X');
                REQUIRE_EQUAL(buffer.get(pos).color(), Color(fg::Yellow, bg::Blue));
            });

            buffer.set(block::Position{0, 1}, Block{U'界', fg::Green, bg::Black});
            REQUIRE_EQUAL(buffer.get(block::Position{0, 1}), U'界');
            REQUIRE(buffer.get(block::Position{1, 1}).isEmpty());
            REQUIRE_EQUAL(buffer.get(block::Position{1, 1}).color(), Color(fg::Green, bg::Black));

            const auto clone = buffer.clone();
            REQUIRE_NOT_EQUAL(clone, nullptr);
            clone->set(block::Position{2, 0}, Block{U'Z', fg::Red, bg::Black});

            REQUIRE_EQUAL(buffer.get(block::Position{2, 0}), U'X');
            REQUIRE_EQUAL(clone->get(block::Position{2, 0}), U'Z');
            REQUIRE_EQUAL(clone->get(block::Position{2, 0}).color(), Color(fg::Red, bg::Black));
        });

        REQUIRE_THROWS_AS(erbsland::err::ParameterError, RemappedBuffer(block::Size{0, 1}));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, RemappedBuffer(block::Size{1, 0}));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, RemappedBuffer(block::Size{10'001, 1}));
    }

    void testResizeWithPreserveContentKeepsVisibleContent() {
        forEachOrientation([&](const geometry::Orientation orientation) -> void {
            auto buffer = createPatternBuffer(block::Size{3, 2}, orientation);
            auto model = createPatternModel(block::Size{3, 2});

            scramble(buffer, model);
            buffer.resize(block::Size{4, 3}, BufferResizeMode::PreserveContent, Block{U'.'});
            model.resize(block::Size{4, 3}, BufferResizeMode::PreserveContent, Block{U'.'});
            requireMatches(buffer, model, [&]() {
                return std::format("resize preserve expand {}", orientationName(orientation));
            });

            buffer.resize(block::Size{2, 2}, BufferResizeMode::PreserveContent, Block{U'.'});
            model.resize(block::Size{2, 2}, BufferResizeMode::PreserveContent, Block{U'.'});
            requireMatches(buffer, model, [&]() {
                return std::format("resize preserve shrink {}", orientationName(orientation));
            });
        });
    }

    void testResizeWithPreserveContentKeepsVisibleContentForPrimaryAxisChanges() {
        forEachOrientation([&](const geometry::Orientation orientation) -> void {
            auto buffer = createPatternBuffer(block::Size{4, 4}, orientation);
            auto model = createPatternModel(block::Size{4, 4});

            scramble(buffer, model);
            const auto expandedSize =
                orientation == geometry::Orientation::Vertical ? block::Size{4, 5} : block::Size{5, 4};
            buffer.resize(expandedSize, BufferResizeMode::PreserveContent, Block{U'.'});
            model.resize(expandedSize, BufferResizeMode::PreserveContent, Block{U'.'});
            requireMatches(
                buffer, model, [&]() { return std::format("primary-axis expand {}", orientationName(orientation)); });

            const auto shrunkSize =
                orientation == geometry::Orientation::Vertical ? block::Size{4, 3} : block::Size{3, 4};
            buffer.resize(shrunkSize, BufferResizeMode::PreserveContent, Block{U'.'});
            model.resize(shrunkSize, BufferResizeMode::PreserveContent, Block{U'.'});
            requireMatches(
                buffer, model, [&]() { return std::format("primary-axis shrink {}", orientationName(orientation)); });
        });
    }

    void testResizeWithPreserveContentFallsBackToFullPreserveForCrossAxisChanges() {
        forEachOrientation([&](const geometry::Orientation orientation) -> void {
            auto buffer = createPatternBuffer(block::Size{4, 3}, orientation);
            auto model = createPatternModel(block::Size{4, 3});

            scramble(buffer, model);
            const auto changedCrossAxisSize =
                orientation == geometry::Orientation::Vertical ? block::Size{5, 3} : block::Size{4, 4};
            buffer.resize(changedCrossAxisSize, BufferResizeMode::PreserveContent, Block{U'.'});
            model.resize(changedCrossAxisSize, BufferResizeMode::PreserveContent, Block{U'.'});
            requireMatches(
                buffer, model, [&]() { return std::format("primary-axis fallback {}", orientationName(orientation)); });
        });
    }

    void testFastResizeKeepsBufferUsableAfterScrambledMaps() {
        forEachOrientation([&](const geometry::Orientation orientation) -> void {
            auto buffer = createPatternBuffer(block::Size{4, 3}, orientation);
            auto model = createPatternModel(block::Size{5, 4});

            scramble(buffer);
            buffer.resize(block::Size{5, 4});
            REQUIRE_EQUAL(buffer.size(), block::Size(5, 4));

            buffer.fill(Block{U'.', fg::White, bg::Black});
            fillPattern(buffer);
            requireMatches(
                buffer, model, [&]() { return std::format("fast resize refill {}", orientationName(orientation)); });
        });
    }

    void testShiftRotateInsertEraseAndMoveMatchReferenceModel() {
        forEachOrientation([&](const geometry::Orientation orientation) -> void {
            auto buffer = createPatternBuffer(block::Size{5, 4}, orientation);
            auto model = createPatternModel(block::Size{5, 4});

            buffer.shift(block::Direction::NorthEast, Block{U'.'}, 1);
            model.shift(block::Direction::NorthEast, Block{U'.'}, 1);
            requireMatches(
                buffer, model, [&]() { return std::format("shift northeast {}", orientationName(orientation)); });

            buffer.rotate(block::Direction::SouthWest, 2);
            model.rotate(block::Direction::SouthWest, 2);
            requireMatches(
                buffer, model, [&]() { return std::format("rotate southwest {}", orientationName(orientation)); });

            buffer.insertRows(blockCoordinate(1), Block{U'+'}, 2);
            model.insertRows(blockCoordinate(1), Block{U'+'}, 2);
            requireMatches(
                buffer, model, [&]() { return std::format("insert rows {}", orientationName(orientation)); });

            buffer.eraseColumns(blockCoordinate(2), Block{U'-'}, 2);
            model.eraseColumns(blockCoordinate(2), Block{U'-'}, 2);
            requireMatches(
                buffer, model, [&]() { return std::format("erase columns {}", orientationName(orientation)); });

            buffer.moveRows(blockCoordinate(1), 2, blockCoordinate(-1), Block{U'#'});
            model.moveRows(blockCoordinate(1), 2, blockCoordinate(-1), Block{U'#'});
            requireMatches(buffer, model, [&]() { return std::format("move rows {}", orientationName(orientation)); });

            buffer.moveColumns(blockCoordinate(1), 2, blockCoordinate(2), Block{U'!'});
            model.moveColumns(blockCoordinate(1), 2, blockCoordinate(2), Block{U'!'});
            requireMatches(
                buffer, model, [&]() { return std::format("move columns {}", orientationName(orientation)); });
        });
    }

    void testMoveOperationsHandleOverflow() {
        forEachOrientation([&](const geometry::Orientation orientation) -> void {
            auto buffer = createPatternBuffer(block::Size{5, 4}, orientation);
            auto model = createPatternModel(block::Size{5, 4});

            buffer.moveRows(blockCoordinate(1), 3, blockCoordinate(-2), Block{U'^'});
            model.moveRows(blockCoordinate(1), 3, blockCoordinate(-2), Block{U'^'});
            requireMatches(
                buffer, model, [&]() { return std::format("move rows overflow up {}", orientationName(orientation)); });

            buffer.moveColumns(blockCoordinate(0), 3, blockCoordinate(3), Block{U'v'});
            model.moveColumns(blockCoordinate(0), 3, blockCoordinate(3), Block{U'v'});
            requireMatches(buffer, model, [&]() {
                return std::format("move columns overflow right {}", orientationName(orientation));
            });
        });
    }

    void testMoveOperationsRecycleFullSpanOverflow() {
        forEachOrientation([&](const geometry::Orientation orientation) -> void {
            {
                auto buffer = createPatternBuffer(block::Size{5, 4}, orientation);
                auto model = createPatternModel(block::Size{5, 4});
                buffer.moveRows(
                    blockCoordinate(0), buffer.size().height().toRawValue(), -buffer.size().height(), Block{U'^'});
                model.moveRows(
                    blockCoordinate(0), model.size().height().toRawValue(), -model.size().height(), Block{U'^'});
                requireMatches(buffer, model, [&]() {
                    return std::format("move all rows above {}", orientationName(orientation));
                });
            }
            {
                auto buffer = createPatternBuffer(block::Size{5, 4}, orientation);
                auto model = createPatternModel(block::Size{5, 4});
                buffer.moveRows(
                    blockCoordinate(0), buffer.size().height().toRawValue(), buffer.size().height(), Block{U'v'});
                model.moveRows(
                    blockCoordinate(0), model.size().height().toRawValue(), model.size().height(), Block{U'v'});
                requireMatches(buffer, model, [&]() {
                    return std::format("move all rows below {}", orientationName(orientation));
                });
            }
            {
                auto buffer = createPatternBuffer(block::Size{5, 4}, orientation);
                auto model = createPatternModel(block::Size{5, 4});
                buffer.moveColumns(
                    blockCoordinate(0), buffer.size().width().toRawValue(), -buffer.size().width(), Block{U'<'});
                model.moveColumns(
                    blockCoordinate(0), model.size().width().toRawValue(), -model.size().width(), Block{U'<'});
                requireMatches(buffer, model, [&]() {
                    return std::format("move all columns left {}", orientationName(orientation));
                });
            }
            {
                auto buffer = createPatternBuffer(block::Size{5, 4}, orientation);
                auto model = createPatternModel(block::Size{5, 4});
                buffer.moveColumns(
                    blockCoordinate(0), buffer.size().width().toRawValue(), buffer.size().width(), Block{U'>'});
                model.moveColumns(
                    blockCoordinate(0), model.size().width().toRawValue(), model.size().width(), Block{U'>'});
                requireMatches(buffer, model, [&]() {
                    return std::format("move all columns right {}", orientationName(orientation));
                });
            }
        });
    }

    void testInvalidArgumentsAreRejected() {
        auto buffer = RemappedBuffer{block::Size{4, 3}, geometry::Orientation::Vertical};

        REQUIRE_THROWS_AS(erbsland::err::ParameterError, buffer.resize(block::Size{0, 3}));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, buffer.shift(block::Direction::North, Block::space(), -1));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, buffer.rotate(block::Direction::East, 5));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, buffer.eraseRows(blockCoordinate(2), Block::space(), 2));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, buffer.eraseColumns(blockCoordinate(-1), Block::space(), 1));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, buffer.insertRows(blockCoordinate(2), Block::space(), 2));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, buffer.insertColumns(blockCoordinate(3), Block::space(), 2));
        REQUIRE_THROWS_AS(
            erbsland::err::ParameterError, buffer.moveRows(blockCoordinate(1), 3, blockCoordinate(1), Block::space()));
        REQUIRE_THROWS_AS(
            erbsland::err::ParameterError,
            buffer.moveColumns(blockCoordinate(3), 2, blockCoordinate(-1), Block::space()));
    }

    void testInvalidArgumentsReportTheParameterName() {
        auto buffer = RemappedBuffer{block::Size{4, 3}, geometry::Orientation::Vertical};
        try {
            buffer.eraseColumns(blockCoordinate(-1), Block::space(), 1);
            REQUIRE(false);
        } catch (const erbsland::err::ParameterError &error) {
            REQUIRE_EQUAL(error.toString(), "The start coordinate is out of bounds. (parameter: startColumn)"_el);
        }
    }

    void testStressOperationsAgainstReferenceModel() {
        forEachOrientation([&](const geometry::Orientation orientation) -> void {
            auto buffer = createPatternBuffer(block::Size{6, 5}, orientation);
            auto model = createPatternModel(block::Size{6, 5});
            auto rng =
                std::mt19937{0xC0DE1234U + static_cast<uint32_t>(orientation == geometry::Orientation::Horizontal)};

            for (int step = 0; step < 250; ++step) {
                const auto operation = randomInt(rng, 0, 8);
                switch (operation) {
                case 0: {
                    const auto direction = randomCardinalOrDiagonal(rng);
                    const auto count = maxDirectionalCount(model.size(), direction) == 0
                        ? 0
                        : randomInt(rng, 0, maxDirectionalCount(model.size(), direction));
                    const auto fillChar = Block{static_cast<char32_t>(U'a' + (step % 26))};
                    buffer.shift(direction, fillChar, count);
                    model.shift(direction, fillChar, count);
                    break;
                }
                case 1: {
                    const auto direction = randomCardinalOrDiagonal(rng);
                    const auto count = maxDirectionalCount(model.size(), direction) == 0
                        ? 0
                        : randomInt(rng, 0, maxDirectionalCount(model.size(), direction));
                    buffer.rotate(direction, count);
                    model.rotate(direction, count);
                    break;
                }
                case 2: {
                    const auto height = model.size().height().toRawValue();
                    const auto count = randomInt(rng, 0, height);
                    const auto start = count == 0 ? height : randomInt(rng, 0, height - count);
                    const auto fillChar = Block{static_cast<char32_t>(U'A' + (step % 26))};
                    buffer.eraseRows(blockCoordinate(start), fillChar, count);
                    model.eraseRows(blockCoordinate(start), fillChar, count);
                    break;
                }
                case 3: {
                    const auto width = model.size().width().toRawValue();
                    const auto count = randomInt(rng, 0, width);
                    const auto start = count == 0 ? width : randomInt(rng, 0, width - count);
                    const auto fillChar = Block{static_cast<char32_t>(U'0' + (step % 10))};
                    buffer.insertColumns(blockCoordinate(start), fillChar, count);
                    model.insertColumns(blockCoordinate(start), fillChar, count);
                    break;
                }
                case 4: {
                    const auto height = model.size().height().toRawValue();
                    const auto count = randomInt(rng, 0, height);
                    const auto start = count == 0 ? height : randomInt(rng, 0, height - count);
                    const auto delta = randomInt(rng, -height, height);
                    const auto fillChar = Block{static_cast<char32_t>(U'k' + (step % 10))};
                    buffer.moveRows(blockCoordinate(start), count, blockCoordinate(delta), fillChar);
                    model.moveRows(blockCoordinate(start), count, blockCoordinate(delta), fillChar);
                    break;
                }
                case 5: {
                    const auto width = model.size().width().toRawValue();
                    const auto count = randomInt(rng, 0, width);
                    const auto start = count == 0 ? width : randomInt(rng, 0, width - count);
                    const auto delta = randomInt(rng, -width, width);
                    const auto fillChar = Block{static_cast<char32_t>(U'p' + (step % 10))};
                    buffer.moveColumns(blockCoordinate(start), count, blockCoordinate(delta), fillChar);
                    model.moveColumns(blockCoordinate(start), count, blockCoordinate(delta), fillChar);
                    break;
                }
                case 6: {
                    const auto newSize = block::Size{
                        randomInt(rng, 1, std::max(1, model.size().width().toRawValue() + 1)),
                        randomInt(rng, 1, std::max(1, model.size().height().toRawValue() + 1))};
                    const auto fillChar = Block{static_cast<char32_t>(U'Z' - (step % 20))};
                    buffer.resize(newSize, BufferResizeMode::PreserveContent, fillChar);
                    model.resize(newSize, BufferResizeMode::PreserveContent, fillChar);
                    break;
                }
                case 7: {
                    const auto pos = block::Position{
                        randomInt(rng, 0, model.size().width().toRawValue() - 1),
                        randomInt(rng, 0, model.size().height().toRawValue() - 1)};
                    const auto value = Block{static_cast<char32_t>(U'!' + (step % 60))};
                    buffer.set(pos, value);
                    model.set(pos, value);
                    break;
                }
                case 8: {
                    const auto fillChar = Block{static_cast<char32_t>(U'.' + (step % 20))};
                    buffer.fill(fillChar);
                    model.fill(fillChar);
                    break;
                }
                default:
                    REQUIRE(false);
                }
                requireMatches(buffer, model, [&]() {
                    return std::format("stress step {} {}", step, orientationName(orientation));
                });
            }
        });
    }
};
