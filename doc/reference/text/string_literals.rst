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

    constexpr auto label = u8"Status"_el;        // U8StringLiteral<char8_t>
    auto labelText = el::U8String{label};         // owning read-only value
    auto labelEditor = el::U8StringEditor{label}; // begin an explicit mutable workflow
    labelEditor.append(u8": ready"_el);

Use ``"_el"`` when you want a constexpr-capable :cpp:class:`U8StringLiteral <erbsland::text::U8StringLiteral>`.
Pass the literal directly when an API accepts it.
Otherwise construct :cpp:class:`U8String <erbsland::text::U8String>` explicitly.
Construct :cpp:class:`U8StringEditor <erbsland::text::U8StringEditor>` when the literal begins an explicit in-place edit
or local construction workflow.

Interface
=========

.. doxygenfunction:: erbsland::text::literals::operator""_el(const char *data, const std::size_t size) noexcept -> U8StringLiteral<char>

.. doxygenfunction:: erbsland::text::literals::operator""_el(const char8_t *data, const std::size_t size) noexcept -> U8StringLiteral<char8_t>

.. doxygenfunction:: erbsland::text::literals::operator""_el(const char16_t *data, const std::size_t size) noexcept -> U16StringLiteral

.. doxygenfunction:: erbsland::text::literals::operator""_el(const char32_t *data, const std::size_t size) noexcept -> U32StringLiteral
.. doxygentypedef:: erbsland::text::StringLiteral
.. doxygenclass:: erbsland::text::U16StringLiteral
    :members:
.. doxygenclass:: erbsland::text::U32StringLiteral
    :members:
.. doxygenclass:: erbsland::text::U8StringLiteral
    :members:
