// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ReferenceBuffer.hpp"

#include "../../support/BufferTestHelper.hpp"

#include <erbsland/block/StdFormat.hpp>
#include <erbsland/text/StdFormat.hpp>

#include <algorithm>
#include <array>
#include <format>
#include <limits>
#include <random>
#include <string>

/// Provides shared setup, randomized input, and model comparisons for remapped-buffer tests.
/// @notest{This helper is exercised by the test suites that inherit from it.}
class RemappedBufferTestSupport : public UNITTEST_SUBCLASS(BufferTestHelper) {
public:
protected:
    /// Run a callback once for each supported buffer orientation.
    template <typename Fn>
    void forEachOrientation(Fn fn) {
        for (const auto orientation : cOrientations) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void { fn(orientation); },
                [&]() -> std::string { return std::format("orientation = {}", orientationName(orientation)); });
        }
    }

    /// Convert a buffer orientation into its diagnostic name.
    [[nodiscard]] static auto orientationName(const geometry::Orientation orientation) -> std::string {
        return orientation == geometry::Orientation::Vertical ? "vertical" : "horizontal";
    }

    /// Create a remapped buffer containing the shared test pattern.
    [[nodiscard]] static auto createPatternBuffer(const block::Size size, const geometry::Orientation orientation)
        -> RemappedBuffer {
        auto buffer = RemappedBuffer{size, orientation};
        fillPattern(buffer);
        return buffer;
    }

    /// Create a reference buffer containing the shared test pattern.
    [[nodiscard]] static auto createPatternModel(const block::Size size) -> ReferenceBuffer {
        auto buffer = ReferenceBuffer{size};
        fillPattern(buffer);
        return buffer;
    }

    /// Fill a buffer with a deterministic alternating character pattern.
    template <typename T>
    static void fillPattern(T &buffer) {
        auto index = 0;
        buffer.size().forEach([&](const block::Position pos) -> void {
            const auto codePoint = static_cast<char32_t>(U'A' + (index % 26));
            buffer.set(pos, Block{codePoint, (index % 2 == 0) ? fg::Green : fg::Cyan, bg::Black});
            index += 1;
        });
    }

    /// Apply matching content operations to a buffer and its reference model.
    void scramble(RemappedBuffer &buffer, ReferenceBuffer &model) {
        buffer.rotate(block::Direction::South, 1);
        model.rotate(block::Direction::South, 1);
        buffer.insertColumns(blockCoordinate(1), Block{U'+'}, 1);
        model.insertColumns(blockCoordinate(1), Block{U'+'}, 1);
        buffer.eraseRows(blockCoordinate(0), Block{U'-'}, 1);
        model.eraseRows(blockCoordinate(0), Block{U'-'}, 1);
        buffer.moveColumns(
            blockCoordinate(0), std::min(2, buffer.size().width().toRawValue()), blockCoordinate(1), Block{U'*'});
        model.moveColumns(
            blockCoordinate(0), std::min(2, model.size().width().toRawValue()), blockCoordinate(1), Block{U'*'});
        requireMatches(buffer, model, []() -> std::string { return "scramble"; });
    }

    /// Apply content operations to a remapped buffer without a reference-model check.
    void scramble(RemappedBuffer &buffer) {
        buffer.rotate(block::Direction::East, 1);
        buffer.insertRows(blockCoordinate(1), Block{U'+'}, 1);
        buffer.eraseColumns(blockCoordinate(0), Block{U'-'}, 1);
        buffer.moveRows(
            blockCoordinate(0), std::min(2, buffer.size().height().toRawValue()), blockCoordinate(1), Block{U'*'});
    }

    /// Require a buffer to match a reference model with contextual diagnostics.
    template <typename tTrace>
    void requireMatches(const ReadableBuffer &buffer, const ReferenceBuffer &model, tTrace &&trace) {
        runWithContext(
            SOURCE_LOCATION(),
            [&]() -> void {
                REQUIRE_EQUAL(buffer.size(), model.size());
                model.size().forEach([&](const block::Position pos) -> void {
                    runWithContext(
                        SOURCE_LOCATION(),
                        [&]() -> void { REQUIRE_EQUAL(buffer.get(pos), model.get(pos)); },
                        [&]() -> std::string { return std::format("{} at {}", trace(), pos); });
                });
            },
            [&]() -> std::string {
                auto message = std::format("trace: {}\nexpected rows:\n", trace());
                for (const auto &row : renderRows(model)) {
                    message += std::format("  {}\n", row);
                }
                message += "actual rows:\n";
                for (const auto &row : renderRows(buffer)) {
                    message += std::format("  {}\n", row);
                }
                return message;
            });
    }

    /// Generate a uniformly distributed integer in an inclusive range.
    [[nodiscard]] static auto randomInt(std::mt19937 &rng, const int minValue, const int maxValue) -> int {
        auto distribution = std::uniform_int_distribution<int>{minValue, maxValue};
        return distribution(rng);
    }

    /// Generate a random cardinal or diagonal direction.
    [[nodiscard]] static auto randomCardinalOrDiagonal(std::mt19937 &rng) -> block::Direction {
        constexpr auto directions = std::array{
            block::Direction::North,
            block::Direction::NorthEast,
            block::Direction::East,
            block::Direction::SouthEast,
            block::Direction::South,
            block::Direction::SouthWest,
            block::Direction::West,
            block::Direction::NorthWest};
        return directions[static_cast<std::size_t>(randomInt(rng, 0, static_cast<int>(directions.size()) - 1))];
    }

    /// Get the maximum valid operation count for a direction and buffer size.
    [[nodiscard]] static auto maxDirectionalCount(const block::Size size, const block::Direction direction) -> int {
        auto maximum = std::numeric_limits<int>::max();
        if (direction.contains(block::Direction::North) || direction.contains(block::Direction::South)) {
            maximum = std::min(maximum, size.height().toRawValue());
        }
        if (direction.contains(block::Direction::West) || direction.contains(block::Direction::East)) {
            maximum = std::min(maximum, size.width().toRawValue());
        }
        return maximum == std::numeric_limits<int>::max() ? 0 : maximum;
    }

private:
    static constexpr auto cOrientations =
        std::array{geometry::Orientation::Vertical, geometry::Orientation::Horizontal};
};
