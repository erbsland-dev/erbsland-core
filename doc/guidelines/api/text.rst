**************************
Text Domain API Guidelines
**************************

These guidelines extend the common API guidelines for public text-processing APIs.

Core Semantics
==============

Read-only Values, Editors and Literals
--------------------------------------

*   ``String`` is the primary owning, read-only text value.
*   ``StringEditor`` is the explicit mutable counterpart type.
*   ``StringLiteral`` is a ``constexpr`` static string literal, that can be held as reference by ``String``.

.. code-block:: text

    String  // parameters, stored text, slices, keys, completed output and ordinary values
    StringEditor  // text under active mutation and explicit mutable handoff
    "text"_el, StringLiteral  // primary string literal

The width-specific names follow the same rule:

.. code-block:: text

    U❮width❯String  // owning read-only UTF-8, UTF-16 or UTF-32 value
    U❮width❯StringEditor  // mutable UTF-8, UTF-16 or UTF-32 editor
    U❮width❯StringLiteral  // constexpr reference to a string literal, created with ``""_el``

``String``, ``StringEditor`` and ``StringLiteral`` are aliases for their UTF-8 counterparts.
``AnyString`` and ``AnyStringEditor`` hold one of the three width-specific read-only or editor types respectively.

Char Means Unicode Code Point
-----------------------------

:cpp:class:`Char <erbsland::text::Char>` represents one Unicode code point, not a grapheme cluster, glyph or byte.
Use ``character`` in parameter names and documentation unless signal values are explicitly part of the contract.

Indexes and Ranges
------------------

Strings use unit-locked indexes, lengths and ranges.
UTF-8 uses ``ByteIndex`` and ``ByteLength`` natively, UTF-16 uses ``U16DataIndex`` and ``U16DataLength``, and UTF-32
uses ``CpIndex`` and ``CpLength``.
All widths also provide explicit code-point access where appropriate.
The string API has methods to convert between ``Cp...`` units and the native units.
The unit type API has methods to convert between ``...Index``, ``...Length/Count`` and ``...Offset``.

Comparison Functions
--------------------

Text comparison, search and count methods take an optional :cpp:type:`CharCompareFn <erbsland::text::CharCompareFn>`.
An empty function uses decoded code-point comparison, there are predefined methods in ``Char`` for ASCII and Unicode
case-folded comparison.
``CaseSensitivity`` selects exact or case-folded comparison and provides callbacks for Unicode or ASCII-only folding.

``AnyString`` compares read-only strings across UTF-8, UTF-16 and UTF-32 without converting their storage.
Its comparison operators accept all values that implicitly convert to ``AnyString``, including width-specific strings,
editors and literals.
``AnyStringEditor`` does not provide comparison operators of its own.

Primary Types
=============

.. code-block:: text

    String  // primary owning read-only UTF-8 string
    StringEditor  // primary mutable UTF-8 string
    AnyString  // read-only value of any supported width
    AnyStringEditor  // mutable value of any supported width
    AnyStringBuilder  // width-agnostic incremental string builder
    StringLiteral  // thin UTF-8 wrapper around ``""_el`` literals
    StringCharReader  // width-agnostic sequential decoded-character reader
    StringSplitter  // sequential zero-copy UTF-8 splitter
    StringConverter  // explicit Erbsland/standard string conversion entry point
    StringDecodeBuffer  // bounded incremental byte-to-text decoder
    StringDecoder  // explicit byte-block-to-string decoder
    StringEncoder  // explicit string-to-byte-block or ring-buffer encoder
    StringFormat  // pattern-based formatter
    StringList, StringEditorList  // read-only and mutable UTF-8 lists
    StringMap, StringHashMap, StringSet, StringHashSet  // read-only UTF-8 keyed collections
    Char, CharRange, CharSet  // Unicode code-point and set types
    TextDocument, TextNode  // mutable semantic text document

Secondary Types
===============

.. code-block:: text

    U8String, U16String, U32String
    U8StringEditor, U16StringEditor, U32StringEditor
    U8StringLiteral, U16StringLiteral, U32StringLiteral
    U8StringList, U16StringList, U32StringList
    U8StringSplitter, U16StringSplitter, U32StringSplitter
    U8StringEditorList, U16StringEditorList, U32StringEditorList
    U8StringMap, U16StringMap, U32StringMap
    U8StringHashMap, U16StringHashMap, U32StringHashMap
    U8StringSet, U16StringSet, U32StringSet
    U8StringHashSet, U16StringHashSet, U32StringHashSet
    U8Format, U16Format, U32Format

Options and Formats
===================

.. code-block:: text

    ByteFormat // options how to format byte blocks as hexadecimal text
    CaseSensitivity // options how to compare case in text APIs
    FloatFormat // options how to format floating-point values
    FloatParseOptions // options how to parse floating-point values
    IntegerBase // base 2,7,10,16 and related methods for representing integers
    StringEncoding // UTF family, byte order and BOM behavior
    TextNodeType // semantic type of a text document node

Typed String Format Specifications
==================================

Typed ``StringFormat`` fields use ``{[index]:selector:options}``.
The trailing colon after the selector is mandatory, including when no options are specified.
Selectors are domain names such as ``text``, ``number``, ``bool``, and ``bytes`` and lock the field to the matching
runtime argument type.

Typed specifications use named, comma-separated options.
Every public option has one globally unique and stable short alias.
A short alias must not be reused by an option in another domain.
Enum-value aliases only need to be unique within their option because they are interpreted after the option name.

Option names, option aliases, enum values, and value aliases use ASCII identifiers and are matched case-insensitively.
New typed domains must use the shared named-option parser and keep selector dispatch extensible for future domains such
as date, datetime, and time.

Utilities
=========

.. code-block:: text

    StringPattern // lightweight decoded-character pattern matcher
    StringSplitMode // discard or keep the separator in returned slices
    CodeSnippet // line-oriented source excerpt with its first line index and optional language
    CodeSnippetMarker // marker range for a line-oriented code snippet
    html::HtmlParser // tolerant HTML to TextDocument parser

Common String Patterns for Any Types
====================================

.. code-block:: text

    o.toString() -> String // ordinary conversions and transformations return read-only strings
    o.toU(8/16/32)String() -> U(8/16/32)String // additionally used when a type supports multiple string widths
    o.toAnyString() -> AnyString // for returning a conversation/transformation that can be of any width
    o.takeString() -> String // after a such call, the type is reset.
    o.toStringEditor() -> StringEditor // special name, for conversion of if an editor is the logical choice
    o.takeStringEditor() -> StringEditor // to pass an internally used editor back to the user

String API Patterns
===================

``TString`` denotes the read-only type for a width and ``TEditor`` its matching editor.

.. code-block:: text

    o.charAt(index/side) -> Char  // access a decoded character or signal
    o.characterLength() -> CpLength  // count decoded code points
    o.compare(text[, compareFn]) -> std::strong_ordering  // compare decoded text
    o.contains(text[, compareFn]) -> bool  // test for text
    o.copy() -> TString  // compact the selected range into independent storage
    o.find(text[, start][, compareFn]) -> ❮index❯  // find text using native indexes
    o.forEach(function) -> util::LoopResult  // visit decoded code points
    o.indexAt(cpIndex/side) -> ❮index❯  // convert to a native position
    o.slice(range/side) -> TString  // create a shared read-only slice
    o.splitAt(index) -> std::pair<TString, TString>  // split into read-only values
    o.toCharIndex(dataIndex) -> CpIndex  // convert a native position
    o.toEscaped(format, amount) -> TString  // create escaped read-only text
    o.toSafeString(maximumWidth, flags) -> TString  // create bounded diagnostic text
    o.transformed(function) -> TString  // transform decoded code points
    o.trimmed([characters][, side]) -> TString  // create trimmed text

String Splitter API Patterns
============================

.. code-block:: text

    TStringSplitter{text, character/set[, mode]}  // create an owning sequential splitter
    o.isAtEnd() -> bool  // test if all parts were read
    o.next() -> TString  // read the next shared slice
    o.remaining() -> TString  // access the unread suffix
    o.reset() -> void  // restart at the beginning

StringEditor API Patterns
=========================

Editors provide the read-only observations above and explicit mutation.
Category-preserving editor operations return ``TEditor``.

.. code-block:: text

    TEditor{string/literal}  // create editable storage
    o.append(character/text[, count]) -> TEditor&  // append text
    o.clear() -> TEditor&  // remove all text while keeping capacity
    o.detach() -> void  // ensure exclusive storage
    o.insert(index, text) -> TEditor&  // insert text
    o.remove(range) -> TEditor&  // remove text
    o.replace(range, text) -> TEditor&  // replace text
    o.reserve(capacity) -> void  // reserve native storage units
    o.slice(range/side) -> TEditor  // preserve editability
    o.trim([characters][, side]) -> TEditor&  // trim in place
    o.trimmed([characters][, side]) -> TEditor  // create an editable result

Converter, Decoder and Builder Patterns
=======================================

.. code-block:: text

    StringConverter{text}.toString() -> String
    StringConverter{text}.toU❮width❯String() -> U❮width❯String
    StringConverter{text}.toStdString() -> std::string
    StringDecoder{data}.decode(...) -> String
    StringDecoder{data}.toU❮width❯String(...) -> U❮width❯String
    StringEncoder{text}.encode(...) -> ByteBlock
    StringEncoder{text}.encodedLength(...) -> ByteLength
    StringEncoder{text}.encodeTo(ring, ...) -> util::Result
    o.peek❮Target❯String(maximum) -> ❮Target❯String
    o.take❮Target❯String(maximum) -> ❮Target❯String
    AnyStringBuilder::u❮width❯() -> AnyStringBuilder
    builder.toString() -> String
    builder.takeString() -> String
    builder.toStringEditor() -> StringEditor  // intentional mutable copy
    builder.takeStringEditor() -> StringEditor  // intentional mutable handoff

Text Document Patterns
======================

.. code-block:: text

    TextDocument()  // create an empty document with a valid root
    o.root() -> TextNodePtr  // access the document root
    o.add❮Element❯(...) -> TextNodePtr // add a child element to the root
    o.toString() -> String  // render completed plain text
    PlainTextRenderer{document}.build() -> String  // build completed plain text
    o.appendTo(builder) -> AnyStringBuilder&  // append to an active builder

Header Files
============

.. code-block:: text

    String.hpp  // primary read-only string and literal aliases
    StringEditor.hpp  // primary mutable string alias
    Literals.hpp  // enable ``""_el`` with ``using namespace el::text::literals``
    AnyStringBuilder.hpp  // incrementally build strings
    StringConverter.hpp  // convert between Erbsland and standard strings
    StringDecodeBuffer.hpp  // incrementally decode byte chunks
    StringDecoder.hpp  // decode binary text
    StringEncoder.hpp  // encode Erbsland strings
    StringEncoding.hpp  // select UTF family, byte order and BOM behavior
    StringSplitter.hpp  // sequentially split UTF-8 strings into shared slices
