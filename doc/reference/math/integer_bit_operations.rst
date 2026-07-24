.. index::
    single: Math
    single: Integer Rotation
    single: Byte Order
    single: Endianness

*************************************
Integer Bit and Byte-Order Operations
*************************************

The integer bit operations provide compiler-safe rotations and byte-order conversion for unsigned native integers.
All operations are ``constexpr`` and independent of host byte order, alignment, and aliasing.
They use fixed-extent byte spans so the required number of bytes is part of the function signature.

Interface
=========

.. doxygenconcept:: erbsland::math::UnsignedNativeInteger

.. doxygenfunction:: erbsland::math::rotateLeft(const T value, const int amount) noexcept -> T

.. doxygenfunction:: erbsland::math::rotateRight(const T value, const int amount) noexcept -> T

.. doxygenfunction:: erbsland::math::loadBigEndian(const std::span<const std::byte, sizeof(T)> bytes) noexcept -> T

.. doxygenfunction:: erbsland::math::loadLittleEndian(const std::span<const std::byte, sizeof(T)> bytes) noexcept -> T

.. doxygenfunction:: erbsland::math::storeBigEndian(const T value, const std::span<std::byte, sizeof(T)> bytes) noexcept

.. doxygenfunction:: erbsland::math::storeLittleEndian(const T value, const std::span<std::byte, sizeof(T)> bytes) noexcept
