// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../bgeo/BlockMargins.hpp"
#include "../bgeo/BlockPosition.hpp"
#include "../bgeo/BlockRectangle.hpp"
#include "../bgeo/BlockSize.hpp"
#include "../err/OutOfRangeError.hpp"
#include "../text/String.hpp"
#include "../text/StringView.hpp"

#include <array>
#include <bit>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace erbsland::cterm {

/// A mutable bitmap storing boolean pixels in row-major order.
class Bitmap {
protected:
    /// Raw bitmap pixel storage.
    using Data = std::vector<bool>;

public:
    /// The maximum bitmap size is 4096x4096 pixels.
    static constexpr auto cMaximumBitmapSize = bgeo::BlockSize{4096, 4096};

public:
    /// Create an empty bitmap with the size `(0,0)`.
    Bitmap() = default;
    /// Create a bitmap with the given size and all pixels cleared.
    /// @param size The bitmap dimensions. The size is limited to 4096x4096 (cMaximumBitmapSize).
    explicit Bitmap(const bgeo::BlockSize size) :
        _size{size.limitedWith(cMaximumBitmapSize)}, _data(_size.area().toSizeT(), false) {}

    // defaults
    ~Bitmap() = default;
    Bitmap(const Bitmap &) = default;
    Bitmap(Bitmap &&) = default;
    auto operator=(const Bitmap &) -> Bitmap & = default;
    auto operator=(Bitmap &&) -> Bitmap & = default;

public: // accessors
    /// Get the bitmap dimensions.
    [[nodiscard]] auto size() const noexcept -> bgeo::BlockSize { return _size; }
    /// Get a rectangle that represents the bitmap bounds.
    [[nodiscard]] auto rect() const noexcept -> bgeo::BlockRectangle {
        return bgeo::BlockRectangle{bgeo::BlockPosition{0, 0}, _size};
    }
    /// Access the raw pixel storage.
    [[nodiscard]] auto data() const noexcept -> const std::vector<bool> & { return _data; }
    /// Access the raw pixel storage for modification.
    [[nodiscard]] auto data() noexcept -> std::vector<bool> & { return _data; }
    /// Read one pixel.
    /// @param pos The pixel position.
    /// @return The pixel state, or `false` if `pos` is outside the bitmap.
    [[nodiscard]] auto pixel(bgeo::BlockPosition pos) const noexcept -> bool;
    /// Read a 2x2 pixel block encoded as four bits.
    /// @param pos The quad position in half-resolution coordinates.
    /// @return Bit mask with top-left/top-right/bottom-left/bottom-right pixels in bits `0..3`.
    [[nodiscard]] auto pixelQuad(bgeo::BlockPosition pos) const noexcept -> uint8_t;
    /// Read the four cardinal pixels as a bit-mask.
    /// Clockwise bit-order: right, down, left, up
    /// @param pos The center pixel position.
    /// @return The bit mask, where bits `0..3` represent the pixels in clockwise order starting from the right.
    [[nodiscard]] auto pixelCardinal(bgeo::BlockPosition pos) const noexcept -> uint8_t;
    /// Read the ring of eight pixels surrounding the given position as a bitmask.
    /// Clockwise bit-order: 0:E, 1:SE, 2:S, 3:SW, 4:W, 5:NW, 6:N, 7:NE
    /// @param pos The center pixel position.
    /// @return The bit mask, where bits `0..7` represent the pixels in clockwise order starting from the right.
    [[nodiscard]] auto pixelRing(bgeo::BlockPosition pos) const noexcept -> uint8_t;
    /// Get a rectangle around all set/cleared pixels.
    /// @param value If true, return the bounding box of set pixels, otherwise cleared pixels.
    /// @return The bounding rectangle of the specified pixels or an empty rectangle if none are set/cleared.
    [[nodiscard]] auto boundingRect(bool value = true) const noexcept -> bgeo::BlockRectangle;
    /// Get the number of set/cleared pixels in this bitmap.
    /// @param value The pixel type to count.
    [[nodiscard]] auto pixelCount(bool value = true) const noexcept -> std::size_t;

public: // modifiers
    /// Set one pixel in the bitmap.
    /// @param pos The pixel position.
    /// @param value The new pixel state.
    /// @note Positions outside the bitmap are ignored.
    void setPixel(bgeo::BlockPosition pos, bool value) noexcept;
    /// Mirror the bitmap horizontally in-place.
    void flipHorizontal() noexcept;
    /// Invert the bitmap in-place.
    void invert() noexcept;

public: // transformation
    /// Get an inverted version of this bitmap.
    /// @return The inverted bitmap.
    [[nodiscard]] auto inverted() const noexcept -> Bitmap;
    /// Convert this bitmap to an outlined version.
    /// The bitmap needs to have a margin large enough for the outline.
    /// Algorithm: If at a cleared source pixel is surrounded by at least one set pixel, the target pixel is set.
    [[nodiscard]] auto outlined() const noexcept -> Bitmap;
    /// Create a new bitmap expanded with a given margin.
    /// Negative margins cut sections from the bitmap.
    /// @param margins The margins for the expansion.
    /// @param value The value to fill the expanded area with.
    /// @return The expanded/shrunk bitmap. If the new width or height is zero, an empty bitmap is returned.
    [[nodiscard]] auto expanded(bgeo::BlockMargins margins, bool value) const noexcept -> Bitmap;

public: // drawing
    /// Draw pixels from a numeric bit mask.
    /// @tparam T The unsigned integer type.
    /// @param position The top left corner where to draw the bit mask.
    /// @param data The array with the data.
    template <typename T>
        requires std::is_integral_v<T> && std::is_unsigned_v<T>
    void draw(bgeo::BlockPosition position, const std::vector<T> &data) noexcept;
    /// Draw another bitmap to the given position.
    /// @param position The top left corner of the bitmap to draw.
    /// @param bitmap The bitmap to draw.
    void draw(const bgeo::BlockPosition position, const Bitmap &bitmap) noexcept {
        bitmap.size().forEach(
            [&](const bgeo::BlockPosition delta) -> void { setPixel(position + delta, bitmap.pixel(delta)); });
    }
    /// Fill a rectangle with a given pixel state.
    /// Positions outside this bitmap are ignored.
    /// @param rect The rectangle to fill.
    /// @param value The new pixel state.
    void fillRect(bgeo::BlockRectangle rect, bool value) noexcept;
    /// Perform a flood fill from the given position.
    /// If the pixel at the given position is already set to the given value, nothing happens.
    /// If the start position is outside the bitmap, nothing happens.
    /// @param pos The start position.
    /// @param value The new pixel state.
    void floodFill(bgeo::BlockPosition pos, bool value) noexcept;

public: // conversion
    /// Build a new bitmap using a function.
    /// @param size The size of the new bitmap.
    /// @param fn The function to use for building the bitmap. Takes a position and returns a boolean.
    /// @return The new bitmap.
    template <typename Fn>
        requires std::is_invocable_r_v<bool, Fn, bgeo::BlockPosition>
    [[nodiscard]] static auto fromFunction(const bgeo::BlockSize size, Fn fn) noexcept -> Bitmap {
        Bitmap bitmap{size};
        bitmap.size().forEach([&](const bgeo::BlockPosition pos) -> void { bitmap.setPixel(pos, fn(pos)); });
        return bitmap;
    }
    /// Create a bitmap from an ASCII pattern.
    /// Each input string becomes one row. Dots (`.`) and spaces create cleared pixels, every other character sets a
    /// pixel. Shorter rows are padded with cleared pixels to the maximum row width.
    /// @param rows The pattern rows to parse.
    /// @return The created bitmap.
    [[nodiscard]] static auto fromPattern(std::initializer_list<text::StringView> rows) -> Bitmap;
    /// Convert this bitmap to a simple ASCII representation.
    /// This function is meant for debugging and visualization.
    /// Use `Buffer` for a true character matrix.
    /// Cleared pixels are represented by dots (`.`), set pixels by a hash (`#`).
    [[nodiscard]] auto toPattern() const -> text::String;

protected:
    /// Access one in-bounds pixel for modification.
    /// @param pos The pixel position.
    /// @throws err::OutOfRangeError if ``pos`` is outside the bitmap.
    [[nodiscard]] auto pixelRef(const bgeo::BlockPosition pos) -> Data::reference {
        if (!_size.contains(pos)) {
            throw err::OutOfRangeError{"bgeo::BlockPosition out of bounds."};
        }
        return _data[_size.index(pos)];
    }

protected:
    /// Bitmap dimensions.
    bgeo::BlockSize _size;
    /// Raw pixel states in row-major order.
    Data _data;
};

template <typename T>
    requires std::is_integral_v<T> && std::is_unsigned_v<T>
void Bitmap::draw(const bgeo::BlockPosition position, const std::vector<T> &data) noexcept {
    for (std::size_t y = 0; y < data.size(); ++y) {
        std::bitset<sizeof(T) * 8> row(data[y]);
        for (auto x = bgeo::BlockCoordinate{0}; x < _size.width(); ++x) {
            setPixel(position + bgeo::BlockPosition{x, bgeo::BlockCoordinate{y}}, row[x.toSizeT()]);
        }
    }
}

}
