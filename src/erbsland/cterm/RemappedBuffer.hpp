// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "WritableBuffer.hpp"

#include "../block/Direction.hpp"
#include "../geometry/Orientation.hpp"
#include "../text/StringLiteral.hpp"

#include <span>

namespace erbsland::cterm {

/// A buffer that allows fast remapping/shifting/inserting/deleting of rows and columns.
///
/// This buffer is useful if you have a large buffer that requires frequent row- or column-based manipulations.
/// It is designed for fast insert/delete/move and shift operations.
///
/// This buffer also knows two orientations: vertical and horizontal.
///
/// - In the vertical orientation, the buffer can efficiently grow vertically, keeping the existing data intact.
/// - In the horizontal orientation, the buffer can efficiently grow horizontally, keeping the existing data intact.
/// - When growing the buffer, memory reallocation may be necessary. Use `reserve` to reserve
///   enough memory to avoid frequent reallocations.
/// - Use `resize(size, BufferResizeMode::PreserveContent, fillChar)` when you need to preserve the visible content.
/// - A preserve-content resize that changes only the primary orientation axis is optimized and fast.
/// - A preserve-content resize that changes the secondary axis must rebuild the logical content and can be expensive.
///
class RemappedBuffer : public WritableBuffer {
public:
    /// Largest buffer size accepted for this buffer.
    constexpr static auto cMaximumSize = block::Size{10'000, 10'000};
    /// Smallest valid buffer size.
    constexpr static auto cMinimumSize = block::Size{1, 1};

    /// A vector mapping one coordinate to another.
    using CoordinateMap = std::vector<block::Coordinate>;

    using WritableBuffer::drawBitmap;
    using WritableBuffer::drawBlockText;
    using WritableBuffer::fill;
    using WritableBuffer::get;
    using WritableBuffer::resize;
    using WritableBuffer::set;

public:
    /// Creates a 1x1 vertical buffer filled with a space.
    /// Usually only used as a placeholder until resized.
    RemappedBuffer();

    /// Construct a buffer with the given size and fill it with an initial block.
    /// @param size The dimensions of the buffer. block::Size must be at least 1x1.
    /// @param orientation The orientation of the buffer. Cannot be changed after creation.
    /// @param fillChar The optional fill character for the buffer.
    /// @throws err::ParameterError if size is invalid.
    explicit RemappedBuffer(
        block::Size size,
        geometry::Orientation orientation = geometry::Orientation::Vertical,
        Block fillChar = Block::space());

    // defaults
    ~RemappedBuffer() override = default;
    RemappedBuffer(const RemappedBuffer &) = default;
    RemappedBuffer(RemappedBuffer &&) = default;
    auto operator=(const RemappedBuffer &) -> RemappedBuffer & = default;
    auto operator=(RemappedBuffer &&) -> RemappedBuffer & = default;

public: // implement ReadableBuffer
    /// Get the current size of the buffer.
    /// @return The configured width and height.
    [[nodiscard]] auto size() const noexcept -> block::Size override;
    /// Get the rectangle covering the whole buffer.
    /// @return A rectangle with origin `(0,0)` and the current size.
    [[nodiscard]] auto rect() const noexcept -> block::Rectangle override;
    /// Read the block stored at the given logical position.
    /// @param pos The logical coordinates inside the buffer.
    /// @return A reference to the stored block, or a shared space block for invalid positions.
    [[nodiscard]] auto get(block::Position pos) const noexcept -> const Block & override;
    /// Create an independent writable copy of this buffer.
    /// @return A shared pointer to the cloned buffer.
    [[nodiscard]] auto clone() const -> WritableBufferPtr override;

public: // implement WritableBuffer
    /// Resize this buffer.
    ///
    /// This is the fast resize path. The internal mapping is rebuilt and the visible content order is undefined after
    /// the operation. Use `resize(size, BufferResizeMode::PreserveContent, fillChar)` if you need to preserve the
    /// visible order.
    /// @param newSize The new size of the buffer.
    /// @throws err::ParameterError if `newSize` is invalid.
    void resize(block::Size newSize) override;
    /// Resize this buffer and optionally keep the visible content order.
    /// A preserve-content resize is fast when only the primary orientation axis changes.
    /// If the secondary axis changes, preserving content requires rebuilding the logical content and is expensive.
    /// @param size The new size.
    /// @param mode `BufferResizeMode::PreserveContent` keeps the visible order and fills new cells with `fillChar`.
    ///   `BufferResizeMode::Fast` resizes using the fastest path and leaves the visible order undefined.
    /// @param fillChar The fill character for newly created cells in preserve-content mode. In fast mode it is only
    ///   used to initialize newly appended storage cells.
    /// @throws err::ParameterError if `size` is invalid.
    void resize(block::Size size, BufferResizeMode mode, Block fillChar) override;
    /// Write a block at the given logical position.
    /// This mirrors the wide-character handling from `Buffer`: zero-width blocks are ignored, width-2 blocks occupy
    /// the next logical cell as an empty continuation cell, and width-2 blocks at the right edge are ignored.
    /// @param pos The logical coordinates within the buffer.
    /// @param block The block to write.
    void set(block::Position pos, const Block &block) noexcept override;

public: // manage memory
    /// Reserve memory for the given buffer size.
    /// @param size The size whose capacity should be reserved.
    void reserve(block::Size size) noexcept;

public: // manipulate the buffer
    /// Shift the buffer in the given direction, fill new cells with a given character.
    /// @param direction The direction of the shift.
    /// @param fillChar The character to fill new cells with.
    /// @param count The number of cells to shift.
    /// @throws err::ParameterError if `count` is negative or exceeds buffer size.
    void shift(block::Direction direction, Block fillChar, int count = 1);
    /// @overload
    void shift(const block::Direction direction, const int count = 1) { shift(direction, Block::space(), count); }
    /// Rotate the buffer in the given direction.
    /// Cells are shifted in a circular manner, wrapping around to the other end of the buffer.
    /// @param direction The direction in which to rotate.
    /// @param count The number of cells to rotate.
    /// @throws err::ParameterError if `count` is negative or exceeds buffer size.
    void rotate(block::Direction direction, int count = 1);
    /// Erase rows in the buffer.
    /// This will erase `count` rows, starting from `startRow`, and insert empty ones at the end.
    /// If you like to actually shrink the buffer, use `resize` *after* this call.
    /// @param startRow The first row to delete.
    /// @param fillChar The character to fill new cells with.
    /// @param count The number of rows to delete.
    /// @throws err::ParameterError if `startRow` is out of bounds or `count` is negative or exceeds buffer size.
    void eraseRows(block::Coordinate startRow, Block fillChar, int count = 1);
    /// @overload
    void eraseRows(const block::Coordinate startRow, const int count = 1) {
        eraseRows(startRow, Block::space(), count);
    }
    /// Erase columns in the buffer.
    /// This will erase `count` columns, starting from `startColumn`, and insert empty ones at the end.
    /// If you like to actually shrink the buffer, use `resize` *after* this call.
    /// @param startColumn The first column to delete.
    /// @param fillChar The character to fill new cells with.
    /// @param count The number of columns to delete.
    /// @throws err::ParameterError if `startColumn` is out of bounds or `count` is negative or exceeds buffer size.
    void eraseColumns(block::Coordinate startColumn, Block fillChar, int count = 1);
    /// @overload
    void eraseColumns(const block::Coordinate startColumn, const int count = 1) {
        eraseColumns(startColumn, Block::space(), count);
    }
    /// Insert rows in the buffer.
    /// The rows at the bottom of the buffer will be erased to make room for the new rows.
    /// If you like to actually grow the buffer, use `resize` *before* this call.
    /// @param startRow The first row to insert.
    /// @param fillChar The character to fill the new rows with.
    /// @param count The number of rows to insert.
    /// @throws err::ParameterError if `startRow` is out of bounds or `count` is negative or exceeds buffer size.
    void insertRows(block::Coordinate startRow, Block fillChar, int count = 1);
    /// @overload
    void insertRows(const block::Coordinate startRow, const int count = 1) {
        insertRows(startRow, Block::space(), count);
    }
    /// Insert columns in the buffer.
    /// The columns on the right side of the buffer will be erased to make room for the new columns.
    /// If you like to actually grow the buffer, use `resize` *before* this call.
    /// @param startColumn The first column to insert.
    /// @param fillChar The character to fill the new columns with.
    /// @param count The number of columns to insert.
    /// @throws err::ParameterError if `startColumn` is out of bounds or `count` is negative or exceeds buffer size.
    void insertColumns(block::Coordinate startColumn, Block fillChar, int count = 1);
    /// @overload
    void insertColumns(const block::Coordinate startColumn, const int count = 1) {
        insertColumns(startColumn, Block::space(), count);
    }
    /// Move rows in the buffer by a given delta.
    /// A positive delta moves rows down, a negative delta moves rows up.
    /// This reshuffles the moved rows, but rows that get moved out of the buffer area are deleted and
    /// replaced by empty cells using the `fillChar`.
    /// @param startRow The first row to move.
    /// @param count The number of rows to move.
    /// @param delta The number of positions to move (positive = down, negative = up).
    /// @param fillChar The character to fill vacated cells with.
    /// @throws err::ParameterError if `startRow` is out of bounds or `count` is negative or exceeds buffer size.
    void moveRows(block::Coordinate startRow, int count, block::Coordinate delta, Block fillChar);
    /// @overload
    void moveRows(const block::Coordinate startRow, const int count, const block::Coordinate delta) {
        moveRows(startRow, count, delta, Block::space());
    }
    /// Move columns in the buffer by a given delta.
    /// A positive delta moves columns right, a negative delta moves columns left.
    /// This reshuffles the moved columns, but columns that get moved out of the buffer area are deleted and
    /// replaced by empty cells using the `fillChar`.
    /// @param startColumn The first column to move.
    /// @param count The number of columns to move.
    /// @param delta The number of positions to move (positive = right, negative = left).
    /// @param fillChar The character to fill vacated cells with.
    /// @throws err::ParameterError if `startColumn` is out of bounds or `count` is negative or exceeds buffer size.
    void moveColumns(block::Coordinate startColumn, int count, block::Coordinate delta, Block fillChar);
    /// @overload
    void moveColumns(const block::Coordinate startColumn, const int count, const block::Coordinate delta) {
        moveColumns(startColumn, count, delta, Block::space());
    }

public:
    /// Fill/clear the buffer with the given character.
    /// @note This will also reset the internal remapping indexes.
    /// @param fillBlock The block to use to fill the buffer.
    void fill(const Block &fillBlock) noexcept override;

private:
    /// Validate the buffer size.
    /// @throws err::ParameterError if size is invalid.
    [[nodiscard]] static auto validatedBufferSize(block::Size size) -> block::Size;
    /// Create a linear index map for the given size.
    /// @param size The size of the buffer.
    /// @return The linear index map.
    [[nodiscard]] static auto linearIndex(std::size_t size) -> CoordinateMap;
    /// Validate a count-only argument.
    /// @param count The count to validate.
    /// @param maximum The maximum accepted count.
    /// @param parameterName The parameter name for error reporting.
    /// @throws err::ParameterError if the count is invalid.
    static void validateCount(int count, int maximum, const text::StringLiteral &parameterName);
    /// Validate a span inside the currently visible range.
    /// @param start The first coordinate in the span.
    /// @param count The number of elements in the span.
    /// @param limit The size of the addressable axis.
    /// @param startName The parameter name for the start coordinate.
    /// @param countName The parameter name for the count.
    /// @throws err::ParameterError if the span is invalid.
    static void validateExistingSpan(
        block::Coordinate start,
        int count,
        int limit,
        const text::StringLiteral &startName,
        const text::StringLiteral &countName);
    /// Validate an insert-style operation on an axis.
    /// @param start The insertion coordinate.
    /// @param count The number of inserted elements.
    /// @param limit The size of the addressable axis.
    /// @param startName The parameter name for the start coordinate.
    /// @param countName The parameter name for the count.
    /// @throws err::ParameterError if the arguments are invalid.
    static void validateInsertArguments(
        block::Coordinate start,
        int count,
        int limit,
        const text::StringLiteral &startName,
        const text::StringLiteral &countName);
    /// Validate a directional count for shift and rotate.
    /// @param direction The direction to validate.
    /// @param count The number of cells to move.
    /// @throws err::ParameterError if the count does not fit the addressed axis.
    void validateDirectionalCount(block::Direction direction, int count) const;
    /// Remap the position.
    /// @param pos The position to remap.
    /// @return The remapped position.
    [[nodiscard]] auto remapPosition(block::Position pos) const noexcept -> block::Position {
        return rect().contains(pos) ? block::Position{_columnRemap[pos.x().toSizeT()], _rowRemap[pos.y().toSizeT()]}
                                    : pos;
    }
    /// Calculate the storage index for a stored coordinate pair.
    /// @param storedX The stored x coordinate.
    /// @param storedY The stored y coordinate.
    /// @return The linear storage index.
    [[nodiscard]] auto storedBufferIndex(block::Coordinate storedX, block::Coordinate storedY) const noexcept
        -> std::size_t {
        if (_orientation == geometry::Orientation::Vertical) {
            return (storedY * _size.width() + storedX).toSizeT();
        }
        return (storedX * _size.height() + storedY).toSizeT();
    }
    /// Calculate the storage index for a logical coordinate pair.
    /// @param x The logical x coordinate.
    /// @param y The logical y coordinate.
    /// @return The linear storage index after remapping.
    [[nodiscard]] auto bufferIndex(block::Coordinate x, block::Coordinate y) const noexcept -> std::size_t {
        return storedBufferIndex(_columnRemap[x.toSizeT()], _rowRemap[y.toSizeT()]);
    }
    /// Get the buffer index for the given orientation and size.
    /// @param pos The stored position.
    /// @param size The size used to linearize the position.
    /// @param orientation The storage orientation.
    /// @return The storage index for `pos`.
    [[nodiscard]] static auto bufferIndex(
        block::Position pos, block::Size size, geometry::Orientation orientation) noexcept -> std::size_t;
    /// Get the storage index for the current size and orientation.
    /// @param pos The position for the index.
    /// @return The buffer index.
    [[nodiscard]] auto bufferIndex(block::Position pos) const noexcept -> std::size_t {
        return storedBufferIndex(pos.x(), pos.y());
    }
    /// Rotate a coordinate map to the logical front or back.
    /// @param map The map to rotate.
    /// @param count The number of elements to rotate.
    /// @param towardFront If `true`, rotate toward index `0`, otherwise toward the logical end.
    static void rotateMap(CoordinateMap &map, int count, bool towardFront) noexcept;
    /// Erase a span from a coordinate map and append the recycled coordinates at the end.
    /// @param map The map to modify.
    /// @param start The first element to erase.
    /// @param count The number of elements to erase.
    /// @return The recycled coordinates that must be refilled.
    [[nodiscard]] static auto eraseFromMap(CoordinateMap &map, block::Coordinate start, int count)
        -> std::span<const block::Coordinate>;
    /// Insert a span into a coordinate map using recycled coordinates from the end.
    /// @param map The map to modify.
    /// @param start The insertion coordinate.
    /// @param count The number of elements to insert.
    /// @return The recycled coordinates that must be refilled.
    [[nodiscard]] static auto insertIntoMap(CoordinateMap &map, block::Coordinate start, int count)
        -> std::span<const block::Coordinate>;
    /// Move a span inside a coordinate map.
    /// @param map The map to transform in place.
    /// @param start The first element to move.
    /// @param count The number of elements to move.
    /// @param delta The movement delta.
    /// @return The recycled coordinates that must be refilled.
    [[nodiscard]] static auto moveInMap(CoordinateMap &map, block::Coordinate start, int count, block::Coordinate delta)
        -> std::span<const block::Coordinate>;
    /// Fill the given stored rows.
    /// @param rows The stored row coordinates to fill.
    /// @param fillChar The fill character.
    void fillStoredRows(std::span<const block::Coordinate> rows, const Block &fillChar) noexcept;
    /// Fill the given stored columns.
    /// @param columns The stored column coordinates to fill.
    /// @param fillChar The fill character.
    void fillStoredColumns(std::span<const block::Coordinate> columns, const Block &fillChar) noexcept;
    /// Execute the fast resize path.
    /// @param newSize The validated new size.
    /// @param fillChar The fill character for newly appended storage cells.
    void fastResize(block::Size newSize, const Block &fillChar);
    /// Execute the fast preserve-content path for primary-axis-only resizes.
    /// @param newSize The validated new size.
    /// @param fillChar The fill character for newly created logical cells.
    void primaryAxisResize(block::Size newSize, const Block &fillChar);
    /// Execute the ordered resize path.
    /// @param newSize The validated new size.
    /// @param fillChar The fill character for newly created logical cells.
    void reorderedResize(block::Size newSize, const Block &fillChar);
    /// Check whether the resize changes only the primary axis of the buffer orientation.
    /// @param newSize The validated new size.
    /// @return `true` if only the orientation axis changes.
    [[nodiscard]] auto isPrimaryAxisOnlyResize(block::Size newSize) const noexcept -> bool;
    /// Access the remap for the orientation axis.
    /// @return The row map for vertical buffers or the column map for horizontal buffers.
    [[nodiscard]] auto primaryMap() noexcept -> CoordinateMap &;
    /// @overload
    [[nodiscard]] auto primaryMap() const noexcept -> const CoordinateMap &;
    /// Copy one stored primary line to another stored primary line.
    /// @param source The stored source row/column coordinate.
    /// @param destination The stored destination row/column coordinate.
    void copyStoredPrimaryLine(block::Coordinate source, block::Coordinate destination) noexcept;

protected:
    block::Size _size;                  ///< The current size of the buffer.
    geometry::Orientation _orientation; ///< The orientation of the buffer layout.
    std::vector<Block> _buffer;         ///< The characters in the buffer.
    CoordinateMap _rowRemap;            ///< A map, `_rowRemap[addressed row] -> stored row`
    CoordinateMap _columnRemap;         ///< A map, `_columnRemap[addressed column] -> stored column`
};

}
