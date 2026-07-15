**************************
Text Domain API Guidelines
**************************

These guidelines extend the Common API Guidelines for public APIs that model text processing functions and classes.

The purpose of this document is to define a base naming vocabulary for math APIs.
It is intentionally plain, technical and list based to get a quick overview of method names and its usage patterns.
If you introduce new vocabulary, update this page to provide a good reference for future extensions.

Core Semantics
==============

Primary Strings
---------------

The primary encoding used by the library is UTF-8. Outside the ``text`` namespace, we use the following aliases and
types:

.. code-block:: text

    StringView  // for parameters, value storage, slices
    String  // building new text, forcing copies
    "text"_el  // primary string literal
    StringFormat  // formatting text
    StringList, StringMap, ... // string based or string-key based containers

UTF-8/UTF-16/UTF-32 Strings
---------------------------

Specialized types for UTF-8/16/32 exist.

.. code-block:: text

    U❮width❯StringView // a safe, all purpose, read-only view of a string that owns the viewed string.
    U❮width❯String // special enforce-copy string for building strings from scratch
    U❮width❯StringCharView // slow code-point-indexed read-only view for UTF-8 and UTF-16 strings.
    U❮width❯Literal // constexpr reference to a string literal, defined as ``""_el``

Char Means Unicode Code Point
-----------------------------

:cpp:class:`Char <erbsland::text::Char>` represents one Unicode code point, not a grapheme cluster, glyph, or byte.
It also reserves invalid values for text reader and character-access signals.
We use ``character`` in parameter names and documentation and mean Unicode code-point unless the API explicitly
documents signal values.

Index, Length, Offset and Range
-------------------------------

We deliberately use unit locked types for indexes, lengths, offsets and ranges.
It makes using a code-point based index for a byte-based index API awkward – which was the goal of that decision.
It makes low-level code in this library awkward too, but that isn't a concern of the user of this library.

.. code-block:: cpp

    String... => ByteIndex, ByteLength
    U8String... => ByteIndex, ByteLength
    U8StringCharView => CpIndex, CpLength
    U16String... => U16DataIndex, U16DataLength
    U16StringCharView => CpIndex, CpLength
    U32String... => CpIndex, CpLength

    auto o.characterLength() -> CpLength

Comparison Functions
--------------------

Text comparison/search/count methods take an optional :cpp:type:`CharCompareFn <erbsland::text::CharCompareFn>`.
Leaving it empty uses the default decoded code-point comparison.
Passing :cpp:func:`Char::compareCaseFolded() <erbsland::text::Char::compareCaseFolded>` enables Unicode simple
case-folded matching where needed without adding separate method names.
:cpp:class:`CharSet <erbsland::text::CharSet>` based APIs are explicit: callers provide the exact accepted characters.

Primary Types
=============

.. code-block:: text

    AnyString // holds either a U8/U16/U32 string, provides common methods
    Char // represents an Unicode code-point and provides extensive tests/conversions/category tools
    CharRange // a range of chars like [a-z]
    CharSet // a set of chars [123abcA-T]
    ByteFormat // options how to format byte blocks as hexadecimal text
    FloatFormat // options how to format floating-point values
    FloatParseOptions // options how to parse floating-point values
    IntegerBase // base 2,7,10,16 and related methods for representing integers
    String // common string (alias for U8String)
    StringBuilder // u8/16/32 agnostic interface to build strings
    StringCharReader // u8/16/32 agnostic interface to read strings `Char` wise (for parsers)
    StringConverter // explicit string/std-string conversion entry point
    StringDecodeBuffer // bounded byte buffer for incremental string decoding
    StringDecoder // explicit byte-block to string decoding entry point
    StringEncoder // explicit string to byte-block encoding entry point
    StringFormat // A pattern based formatter, similar to ``std::format``
    StringHashMap // common string hash map (alias for U8StringHashMap)
    StringCIHashMap // common case-insensitive string hash map (alias for U8StringCIHashMap)
    StringHashSet // common string hash set (alias for U8StringHashSet)
    StringCIHashSet // common case-insensitive string hash set (alias for U8StringCIHashSet)
    StringList // common string list (alias for U8StringList)
    StringLiteral // thin wrapper around `""_el` literals
    StringMap // common string map (alias for U8StringMap)
    StringCIMap // common case-insensitive string map (alias for U8StringCIMap)
    StringPattern // lightweight decoded-character pattern matcher
    StringSet // common string set (alias for U8StringSet)
    StringCISet // common case-insensitive string set (alias for U8StringCISet)
    CodeSnippetMarker // marker range for a line-oriented code snippet
    TextDocument // mutable structured text document with a document root node
    text::html::HtmlParser // tolerant HTML to TextDocument parser
    TextNode // mutable shared node in a text document tree
    TextNodeData // extensible metadata attached to a text document node
    TextNodeType // semantic type of a text document node
    StringView // a read-only owning view of a string (not to be compared with std::string_view)
    StringViewList // common string view list (alias for U8StringViewList)

Secondary Types
===============

.. code-block:: text

    U8String, U16String, U32String
    U8StringHashMap, U16StringHashMap, U32StringHashMap
    U8StringCIHashMap, U16StringCIHashMap, U32StringCIHashMap
    U8StringHashSet, U16StringHashSet, U32StringHashSet
    U8StringCIHashSet, U16StringCIHashSet, U32StringCIHashSet
    U8StringList, U16StringList, U32StringList
    U8StringMap, U16StringMap, U32StringMap
    U8StringCIMap, U16StringCIMap, U32StringCIMap
    U8StringSet, U16StringSet, U32StringSet
    U8StringCISet, U16StringCISet, U32StringCISet
    U8StringView, U16StringView, U32StringView
    U8StringViewList, U16StringViewList, U32StringViewList
    U8StringCharView, U16StringCharView
    U8StringLiteral, U16StringLiteral, U32StringLiteral
    U8Format, U16Format, U32Format

Option Types
============

.. code-block:: text

    BooleanFormat // options how to format boolean values
    IntegerFormat // options how to format integers
    ByteFormat // options how to format byte blocks as hexadecimal text
    FloatFormat // options how to format floating-point values
    IntegerParseOptions // options how to parse an integer
    FloatParseOptions // options how to parse a floating-point value
    SafeStringFlags // options for bounded log/debug-safe string output

Enumerations
============

.. code-block:: text

    AsciiCategory // named ascii character categories
    Capitalization // capitalization for generated words
    CharSignal // named non-character signals stored in Char
    EncodingErrorMode // behavior when decoding invalid byte/word sequences
    EscapeAmount // amount of characters to escaped
    EscapeFormat // target format for escaping
    FloatParseFlag // flags for floating-point parsing
    ByteFormatFlag // flags for byte block hexadecimal formatting
    LetterCase // upper- or lowercase
    SafeStringFlag // flags for bounded log/debug-safe string output
    StringEncoding // byte encodings for strings
    StringSide // front or back side of a string-like value
    StringKind // u8/u16 or u32 string
    TruncateMode // position where truncation removes text
    UnicodeCategory // a Unicode category
    UnicodeCategoryGroup // a UnicodeCategoryGroup

Diagnostic Escaping
===================

Use ``EscapeFormat::Display`` for untrusted text shown to a user.
It preserves printable punctuation and non-ASCII text while representing control and format characters with readable
C-style escape sequences.
Use ``TextNode::addEscapedText(text, format[, amount])`` when building a semantic document: ordinary runs become
``Text`` nodes and every replacement becomes one indivisible ``EscapeSequence`` node.
The operation decodes malformed UTF input tolerantly.

Header Files
============

.. code-block:: text

    Literals.hpp // use with ``using namespace el::text::literals`` to enable ``""_el``.
    StringConverter.hpp // convert between Erbsland and standard string types.
    StringDecodeBuffer.hpp // incrementally decode byte chunks into strings.
    StringDecoder.hpp // decode binary text data into Erbsland strings.
    StringEncoder.hpp // encode Erbsland strings into binary text data.
    UnicodeVersion.hpp // to access the Unicode database version the library uses.

String API Patterns
===================

.. code-block:: text

    T::fromFloat(v, format) -> T // create from a floating-point value.
    T::fromInteger(v, format) -> T // create from an integer.
    T::fromByteBlock(bytes, format) -> T // create hexadecimal text from a byte block.
    T::fromCharacter(character, count) -> T // create text by repeating one code point.
    T::fromJoined({views...}) -> T // create text by joining matching string views without a separator.
    T::swap(first, second) // swap two strings.
    o.advance(index, count) -> bool // advance an index forward by the given count.
    o.append(character/text[, count]) -> T& // append code point(s) or text.
    o.aligned(length, alignment, fill) -> T // pad to the requested code-point length.
    o.begin() ->_iterator // get an iterator to the first decoded character.
    o.capacity() -> ❮index❯ // get the current memory capacity.
    o.charAt(index) -> Char  // access a character or signal at the given location
    o.charAt(side) -> Char  // access the first/last character or null for empty strings.
    o.clear() -> T& // remove all characters, keep capacity.
    o.compare(text[, compareFn]) -> std::strong_ordering // compare with other string.
    o.contains(text[, compareFn]) -> bool // check if the string contains the given text.
    o.containsOneOf(characters) -> bool // check if any character from the set is contained.
    o.containsOnly(characters) -> bool // check if all characters are from the set.
    o.copy() -> TString // materialize a string copy from a view with the same encoding width.
    o.count(text[, compareFn]) -> unit::ElementCount // count non-overlapping text occurrences.
    o.detach() -> void // detach the string data for exclusive access.
    o.end() ->_iterator // get an iterator pointing after the last decoded character.
    o.endsWith(text[, compareFn]) -> bool // check if the string ends with the given text
    o.escapedSize(format, amount) -> ❮index❯ // get the size of the escaped string.
    o.find(text[, start][, compareFn]) -> ❮index❯ // find the first occurrence of the text.
    o.findFirstNotOf(characters[, start]) -> ❮index❯ // find the first character not in the set.
    o.findFirstOf(characters[, start]) -> ❮index❯ // find the first character in the set.
    o.findLastNotOf(characters[, end]) -> ❮index❯ // find the last character not in the set.
    o.findLastOf(characters[, end]) -> ❮index❯ // find the last character in the set.
    o.forEach(function) -> util::LoopResult // call a function for every decoded code point.
    o.indexAt(charIndex) -> ❮index❯ // get the data index of the character at the given char index.
    o.indexAt(side) -> ❮index❯ // get the front or back data index.
    o.isEmpty() -> bool // test if the string is empty.
    o.isValidUtf❮width❯() -> bool // test if the string is valid UTF-8/16/32.
    o.readCharAndAdvance(index) -> Char // read at an index and move it after the character.
    o.readCharAndRetreat(index) -> Char // read before an index and move it to the character start.
    o.insert(index, text) -> T& // insert text at a native or character-based index.
    o.inserted(index, text) -> T // return a copy with text inserted at a native or character-based index.
    o.keep(range) -> T& // keep only a native or character-based range in-place.
    o.kept(range) -> T // return a copy keeping only a native or character-based range.
    o.length() -> ❮index❯ // get the string length in the native data units.
    o.characterLength() -> unit::CpLength // get the decoded code-point length.
    o.memoryUsage() -> unit::ByteLength // get the estimated memory usage.
    o.operator<=>(text) -> std::strong_ordering // compare with other string.
    o.remove(range) -> T& // remove a native or character-based range in-place.
    o.removeAll(characters/text[, compareFn]) -> T& // remove all matching characters or text occurrences in-place.
    o.removeFirst(text[, compareFn]) -> T& // remove the first matching text occurrence in-place.
    o.removed(range) -> T // return a copy with a native or character-based range removed.
    o.removedAll(characters/text[, compareFn]) -> T // return a copy with all matching characters or text removed.
    o.removedFirst(text[, compareFn]) -> T // return a copy with the first matching text occurrence removed.
    o.replace(range, text) -> T& // replace a native or character-based range in-place.
    o.replaceAll(characters/text, replacement[, compareFn]) -> T& // replace characters or all occurrences of text.
    o.replaceFirst(text, replacement[, compareFn]) -> T& // replace the first matching text occurrence in-place.
    o.replaced(range, text) -> T // return a copy with a native or character-based range replaced.
    o.replacedAll(characters/text, replacement[, compareFn]) -> T // return a replaced copy.
    o.replacedFirst(text, replacement[, compareFn]) -> T // return a copy with the first matching text occurrence replaced.
    o.reserve(capacity) -> void // reserve capacity for the string.
    o.reset() -> void // clear all characters and reset capacity to default.
    o.retreat(index, count) -> bool // retreat an index backward by the given count.
    o.shrinkToFit() -> void // free unused memory.
    o.slice(range) -> TView // get a slice of the string.
    o.slice(side, length) -> TView // get the front or back portion of the string.
    o.slice(side, index) -> TView/T // get the text before or after an index.
    o.slice(side) -> std::tuple<Char, TView/T> // remove one character from a side and return it plus the remainder.
    o.splitAt(index) -> std::pair<TView/T, TView/T> // split into the text before and after an index.
    o.startsWith(prefix[, compareFn]) -> bool // check if the string starts with the given prefix.
    o.storageId() -> mem::StorageIdentifier // get a unique identifier for the storage range.
    o.toCharIndex(dataIndex) -> unit::CpIndex // get the char index at the given data index.
    o.toCharView() -> ❮width❯StringCharView // create a character-index-based view.
    o.toEscaped(format, amount) -> T // escape the string.
    o.toHash/toHashCI() -> std::size_t // create a hash value from decoded code points (CI = using simple case folding)
    o.toFloat(defaultValue, options) -> T // convert to a floating-point value, return default on error.
    o.toFloatOrThrow(options) -> T // convert to a floating-point value or throw on error.
    o.toInteger(defaultValue, options) -> T // convert to an integer, return default on error.
    o.toSafeString(maximumWidth, flags) -> T // create bounded escaped text for logs/debug output.
    o.transformed(function) -> T // map every decoded code point through a TransformCharacterFn.
    o.truncate(maximumWidth, mode, ellipsis) -> T& // truncate in-place by decoded code-point width.
    o.truncated(maximumWidth, mode, ellipsis) -> T // return a truncated copy by decoded code-point width.
    o.trim([characters][, side]) -> T& // remove ASCII whitespace or characters from both sides or one side.
    o.trimmed([characters][, side]) -> T // return a trimmed copy or view.

String Converter Patterns
=========================

.. code-block:: text

    T{text}.toString() -> String // convert to the default UTF-8 string.
    T{text}.toStringView() -> StringView // safe view; owns storage if conversion was needed.
    T{text}.toU❮width❯String(errorMode) -> U❮width❯String // convert between Erbsland strings.
    T{text}.toStdString(errorMode) -> std::string // convert to UTF-8 byte string.
    T{text}.toStdU❮width❯String(errorMode) -> std::u❮width❯string // convert to a standard string.
    T{text}.toStdWString(errorMode) -> std::wstring // convert to native wide string.

String Encoder Patterns
=======================

.. code-block:: text

    T{text}.encode(encoding, bomMode) -> mem::ByteBlock // encode an Erbsland string/view/char-view.

String Decoder Patterns
=======================

.. code-block:: text

    T{data}.decode(encoding, bomMode, errorMode) -> String // decode binary text to the default string.
    T{data}.toU❮width❯String(encoding, bomMode, errorMode) -> U❮width❯String // decode to a target width.

String Decode Buffer Patterns
=============================

.. code-block:: text

    StringDecodeBuffer(capacity, encoding, bomMode, errorMode) // create a bounded incremental decoder
    o.availableSpace() -> ByteLength // writable byte capacity
    o.byteLength() -> ByteLength // buffered byte count
    o.decodableCharacters(maximum) -> CpLength // complete characters ready for decoding
    o.codePointStatus() -> CodePointStatus // status of the next code point prefix
    o.write(bytes) -> void // append byte data
    o.finish() -> void // mark input as complete; trailing incomplete data follows errorMode
    o.peek❮Target❯String(maximum) -> ❮Target❯String // decode without consuming bytes
    o.take❮Target❯String(maximum) -> ❮Target❯String // decode and consume the produced bytes

String Builder Patterns
=======================

.. code-block:: text

    T::u❮width❯() -> StringBuilder // create a builder for a fixed target width.
    T::u8(ByteLength) -> StringBuilder // create a UTF-8 builder with native byte capacity.
    T::u16(U16DataLength) -> StringBuilder // create a UTF-16 builder with native code-unit capacity.
    T::u32(CpLength) -> StringBuilder // create a UTF-32 builder with native code-point capacity.
    T::withCapacity(kind, CpLength) -> StringBuilder // create a builder with decoded code-point capacity.
    T::basedOn(text, additionalCapacity) -> StringBuilder // create a builder from existing text.
    o.append(character/text[, count]) -> StringBuilder& // append code point(s) or text.
    o.appendByteBlock(bytes, format) -> StringBuilder& // append a byte block as hexadecimal text.
    o.takeU❮width❯String() -> U❮width❯String // move out a string and reset the builder.
    o.to<T>() -> T // create a supported editable string type.
    o.toU❮width❯String() -> U❮width❯String // create a string copy.

Text Document Patterns
======================

.. code-block:: text

    TextDocument() // create an empty document with a valid root node
    o.root() -> TextNodePtr // access the document root
    o.add❮Element❯(...) -> TextNodePtr // add a child element to the root
    o.toString() -> String // render the document as plain text
    o.addCodeSnippet(lines, startLine, markers, language) -> TextNodePtr // add a numbered code excerpt
    CodeSnippetMarker(line, column, length, label, style) // mark a range in a code excerpt

    PlainTextRenderer{document}.build() -> String // build a plain-text string from a document
    o.appendTo(builder) -> StringBuilder& // append rendered plain text to a builder

    TextNode() // empty/invalid node
    T::create❮Element❯(...) -> TextNodePtr // create a detached instance
    o.add❮Element❯(...) -> TextNodePtr // add a child to this node
    o.children() -> const TextNodeList& // access child nodes
    o.toDiagnosticTree() -> StringTree // inspect the document tree

HTML Parser Patterns
====================

.. code-block:: text

    text::html // namespace for HTML parsing APIs
    HtmlParser{text} // create a parser from AnyStringView
    o.parse() -> TextDocument // tolerant parsing, no parse exceptions
    o.parseOrThrow() -> TextDocument // tolerant parsing, throws ParseError only for unrecoverable future failures

String List API Patterns
========================

.. code-block:: text

    T::fromSplit(text, separators, ElementCount maximumSplits, keepEmpty) -> T // split using matching views.
    o.join(separator) -> TString // join list elements using an optional separator.
