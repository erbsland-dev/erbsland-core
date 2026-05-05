.. index::
    single: String Literals

***************
String Literals
***************

Introduction
============

The literal helpers in ``erbsland::text::literals`` are the preferred way to write static UTF-8 text in Erbsland Core
code.
They keep the type of the literal visible, avoid unsafe pointer-and-size pairs in user code, and let read-only APIs
refer directly to the original literal storage.

.. code-block:: cpp

    using namespace erbsland::text::literals;

    constexpr auto label = u8"Status"_el; // U8StringLiteral<char8_t>
    auto labelView = u8"Status"_elv;      // U8StringView
    auto labelText = u8"Status"_els;      // U8String

Use ``"_el"`` when you want a constexpr-capable :cpp:class:`U8StringLiteral <erbsland::text::U8StringLiteral>`.
Use ``"_elv"`` for APIs that inspect text through :cpp:class:`U8StringView <erbsland::text::U8StringView>`.
Use ``"_els"`` for APIs that need an owning :cpp:class:`U8String <erbsland::text::U8String>`.

Interface
=========

.. doxygenfunction:: erbsland::text::literals::operator""_el(const char *data, const std::size_t size) noexcept -> U8StringLiteral<char>

.. doxygenfunction:: erbsland::text::literals::operator""_el(const char8_t *data, const std::size_t size) noexcept -> U8StringLiteral<char8_t>

.. doxygenfunction:: erbsland::text::literals::operator""_el(const char16_t *data, const std::size_t size) noexcept -> U16StringLiteral

.. doxygenfunction:: erbsland::text::literals::operator""_el(const char32_t *data, const std::size_t size) noexcept -> U32StringLiteral

.. doxygenfunction:: erbsland::text::literals::operator""_elv(const char *data, std::size_t size) noexcept -> U8StringView

.. doxygenfunction:: erbsland::text::literals::operator""_elv(const char8_t *data, std::size_t size) noexcept -> U8StringView

.. doxygenfunction:: erbsland::text::literals::operator""_elv(const char16_t *data, std::size_t size) noexcept -> U16StringView

.. doxygenfunction:: erbsland::text::literals::operator""_elv(const char32_t *data, std::size_t size) noexcept -> U32StringView

.. doxygenfunction:: erbsland::text::literals::operator""_els(const char *data, std::size_t size) -> U8String

.. doxygenfunction:: erbsland::text::literals::operator""_els(const char8_t *data, std::size_t size) -> U8String

.. doxygenfunction:: erbsland::text::literals::operator""_els(const char16_t *data, std::size_t size) -> U16String

.. doxygenfunction:: erbsland::text::literals::operator""_els(const char32_t *data, std::size_t size) -> U32String
.. doxygentypedef:: erbsland::text::StringLiteral
.. doxygenclass:: erbsland::text::U16StringLiteral
    :members:
.. doxygenclass:: erbsland::text::U32StringLiteral
    :members:
.. doxygenclass:: erbsland::text::U8StringLiteral
    :members:
