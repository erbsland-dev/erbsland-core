**************************
Text Domain API Guidelines
**************************

Core Semantics
==============

Character Model
---------------

.. code-block:: text

    character = Unicode code point, not grapheme, glyph, or byte
    UTF-8 native position = byte index or length
    UTF-16 native position = char16 data index or length
    UTF-32 native position = code-point index or length
    cross-width position = code-point index or length
    signal = explicit non-character result from tolerant character access
    normalization = explicit NFC/NFD/NFKC/NFKD with malformed input replaced
    normalization non-starter limit = after decomposition, replace starter plus more than 30 non-starters with U+FFFD
    compatibility normalization = NFKC/NFKD may discard compatibility distinctions

Comparison
----------

.. code-block:: text

    default order = decoded code-point order
    exact comparison = no case folding
    ASCII folding = case-insensitive comparison limited to ASCII letters
    Unicode folding = case-insensitive comparison using Unicode mapping
    cross-width comparison = decoded comparison without storage conversion

Sensitive UTF-8 Storage
-----------------------

.. code-block:: text

    marked allocation = one-way sensitivity metadata shared by every UTF-8 alias
    same-width derived value = preserves the allocation mark
    conversion boundary = other widths, encoded bytes, formatting, escaping, and standard text are unmarked
    comparison = ordinary string comparison without a constant-time guarantee
    release = complete allocation erased after its final alias is released

Primary Types
=============

.. code-block:: text

    String, U8String, U16String, U32String // owning read-only strings for each supported width
    Char // Unicode code-point value with explicit signal states

Processing Types
================

.. code-block:: text

    AnyString, AnyStringEditor, AnyStringBuilder // runtime-width text values and builder
    StringEditor, U8StringEditor, U16StringEditor, U32StringEditor // local mutable editing and construction values
    StringLiteral, U8StringLiteral, U16StringLiteral, U32StringLiteral // compile-time string literals
    StringCharReader, StringCharReaderState // sequential decoded-character reader and retained state
    StringSplitter, U8StringSplitter, U16StringSplitter, U32StringSplitter // owning sequential splitters
    CharRange, CharSet, CombinedChar, CharSignal // code-point range, set, combined value, and access signal
    UnicodeCategory, UnicodeCategoryGroup, AsciiCategory // character classifications
    NormalizationForm // NFC, NFD, NFKC, and NFKD Unicode normalization selection
    StringKind, StringSide, StringSplitMode, CaseSensitivity // width, position, splitting, and comparison policies
    StringConverter // explicit Erbsland Core and standard string conversion entry point
    StringDecoder, StringEncoder, StringDecodeBuffer // byte codecs and bounded incremental decoding
    StringEncoding, EncodingMode, StringBomMode // encoding, error, and byte-order-mark policies

Formatting and Parsing Types
============================

.. code-block:: text

    StringFormat, U8Format, U16Format, U32Format // typed formatters for each output width
    FormatArgument, FormatArgumentKind, FormatAs // erased arguments and text formatting adapters
    BooleanFormat, ByteFormat, IntegerFormat, FloatFormat // scalar formatting policies
    IntegerParseOptions, FloatParseOptions // numeric parsing policies
    ReadIntegerResult, ReadNumberStatus // incremental numeric parsing result and status
    IntegerBase, IntegerSignMode, LetterCase, Capitalization // numeric and text presentation values
    EscapeFormat, EscapeAmount, SafeStringFlags, TruncateMode // escaping and bounded-display policies
    FormatError, ParseNumberError // typed formatting and numeric parsing failures
    EncodingError, U8EncodingError, U16EncodingError, U32EncodingError // width-specific encoding failures

Collection and Pattern Types
============================

.. code-block:: text

    StringList, U8StringList, U16StringList, U32StringList // read-only string lists
    StringEditorList, U8StringEditorList, U16StringEditorList, U32StringEditorList // mutable string lists
    StringMap, StringHashMap, StringSet, StringHashSet // ordered and hashed UTF-8 collections
    StringCIMap, StringCIHashMap, StringCISet, StringCIHashSet // ASCII-case-insensitive UTF-8 collections
    U❮width❯StringMap, U❮width❯StringHashMap // width-specific ordered and hashed maps
    U❮width❯StringSet, U❮width❯StringHashSet // width-specific ordered and hashed sets
    StringPattern // lightweight decoded-character matcher
    StringTree // dot-separated string-key hierarchy

Document Types
==============

.. code-block:: text

    TextDocument, TextNode, TextNodeData // mutable semantic document tree
    TextNodeType, TextWalkStatus, TextWalkResult // node classification and traversal control
    PlainTextRenderer // semantic document to plain-text renderer
    CodeSnippet, CodeSnippetMarker // indexed source excerpt and annotation
    html::HtmlParser // tolerant HTML-to-document parser
    json::JsonValue, json::JsonType // copy-on-write JSON value tree and semantic type
    json::JsonArray, json::JsonObject // ordered JSON containers
    json::JsonParseOptions, json::JsonFormatOptions // JSON limits and output controls

Layout Renderer Types
=====================

.. code-block:: text

    render::Environment, render::EnvironmentOptions, render::RenderLimits // renderer configuration and limits
    render::Loader, render::FileSystemLoader, render::ResourceLoader // logical-layout source providers
    render::LayoutSource // source text, diagnostic origin, and opaque revision
    render::Context // concrete named values passed to a render
    render::Value, render::ValueType // immutable shared render values and their semantic type
    render::FilterFn // callback receiving the piped value and up to two arguments in one immutable value list
    render::RenderError, render::RenderErrorContext // structured user-facing render failures
    render::RenderErrorCategory // stable failure classification

Codec Types
===========

.. code-block:: text

    base_n::BaseNFormat // alphabet, padding, whitespace, and line-wrapping policy
    base_n::BaseNEncoder, base_n::BaseNDecoder // binary-to-text and text-to-binary codecs
    base_n::BaseNFormatFlag, base_n::BaseNFormatFlags // padding and wrapping flags
    punycode::PunycodeEncoder, punycode::PunycodeDecoder // Unicode/Punycode and optional strict IDNA2008 conversion
    punycode::PunycodeOptions, punycode::PunycodeMode // pure, label, or domain processing policy

Pattern Definitions
===================

.. code-block:: text

    E = StringEditor/U❮width❯StringEditor // mutable string for the selected width
    I = unit::❮Index❯ // native or code-point string index
    S = String/U❮width❯String // read-only string for the selected width

String Value Patterns
=====================

.. code-block:: text

    o.length()/characterLength()/isEmpty() -> T // inspect native-unit and code-point bounds
    o.charAt(index-or-side) -> Char // access a decoded character or signal
    o.indexAt(cpIndex-or-side)/toCharIndex(nativeIndex) -> I // convert between native and code-point positions
    o.slice(range-or-side)/copy() -> S // create a shared slice or compact independent value
    o.compare(text[, comparison]) -> std::strong_ordering // compare decoded text
    o.find/findLast(text[, start, comparison]) -> I // locate text using native indexes
    o.startsWith/endsWith/contains(text[, comparison]) -> bool // test text membership
    o.containsOnly(set-or-ascii-category) -> bool // validate decoded character membership
    o.forEach(function) -> util::LoopResult // visit decoded code points
    o.splitAt(index) -> std::pair❮S❯ // split into read-only values
    o.trimmed/transformed([arguments]) -> S // return processed read-only text
    o.normalized(form) -> S // explicitly normalize and share storage when already normalized
    o.toEscaped/toSafeString([options]) -> S // create escaped or bounded diagnostic text

String Editing Patterns
=======================

.. code-block:: text

    T(string-or-literal) // create explicit editable storage
    o.append/insert/replace(position, text) -> E& // add or replace text in place
    o.remove/trim(range-or-characters) -> E& // remove text in place
    o.slice/trimmed(range-or-characters) -> E // return an editable result
    o.normalize(form) -> E& // normalize in place
    o.normalized(form) -> E // return an editable normalized result
    o.clear()/reset() // empty while retaining or releasing storage
    o.reserve(capacity)/detach() // prepare storage or ensure exclusive ownership
    o.toString/takeString() -> String // copy or move completed UTF-8 text
    o.toStringEditor/takeStringEditor() -> StringEditor // copy or move mutable UTF-8 storage

Reader and Split Patterns
=========================

.. code-block:: text

    T(text[, separator, mode]) // create an owning reader or splitter
    o.isAtEnd()/position()/remaining() -> T // inspect sequential state
    o.next()/peek() -> T // consume or inspect the next character or slice
    o.advanceIf(character-or-string[, compare]) -> bool // transactionally skip an optional token
    o.advanceWhile/advanceUntil(set-or-ascii-category[, maximum]) -> unit::CpLength // skip and inspect the count
    o.readWhile/readUntil(function, set-or-ascii-category[, maximum]) -> util::LoopResult // decoded scan
    o.skip() // discard the next splitter part without creating a slice
    o.reset([state]) // restart or restore retained reader state

Conversion and Encoding Patterns
================================

.. code-block:: text

    T(input[, encoding]) // create a conversion, decoder, or encoder operation
    o.toString/toU❮width❯String/toStdString([mode]) -> T // convert with tolerant or strict decoding
    o.toAnyString() -> AnyString // preserve runtime-selected width
    o.decode([mode]) -> S // decode owned bytes to the requested width
    o.validateOrThrow(encoding[, bom-mode]) // strictly validate encoded bytes without creating a string
    o.encode() -> mem::ByteBlock // tolerantly encode text into owned bytes
    o.encodedLength() -> unit::ByteLength // calculate encoded byte length
    o.encodeTo(ring) -> util::Result // atomically encode into a byte ring
    o.peek❮Width❯String/take❮Width❯String(maximum) -> S // inspect or consume bounded decoded text
    o.setSensitive(enabled) // configure StringDecodeBuffer storage and UTF-8 result marking

Formatting and Parsing Patterns
===============================

.. code-block:: text

    T(pattern) // create a typed formatter
    o.format(arguments) -> S // format checked runtime arguments
    T::format(pattern, arguments) -> S // format without retaining a formatter
    o.toBoolean/toInteger/toFloat([fallback-or-options]) -> T // parse with a fallback
    o.toBooleanOrThrow/toIntegerOrThrow/toFloatOrThrow([options]) -> T // parse or throw
    o.toString() -> String // create a canonical option or policy representation
    T::fromString/fromStringOrThrow(text) -> T // parse a canonical option or policy representation

Sensitive UTF-8 Patterns
========================

.. code-block:: text

    o.isSensitive() -> bool // inspect the shared UTF-8 allocation mark
    o.markAsSensitive() // irreversibly mark the complete shared UTF-8 allocation
    o.copy/slice/trimmed/transformed(...) -> S // preserve the source mark for same-width results
    o.reset() // release this alias; erase marked storage after the final alias releases it

Document Patterns
=================

.. code-block:: text

    T() // create an empty document with a valid root
    o.root()/add❮Element❯(arguments) -> TextNodePtr // access or extend the document tree
    o.walk(callback) -> TextWalkResult // traverse semantic nodes
    o.toString() -> String // render completed plain text
    o.appendTo(builder) -> AnyStringBuilder& // append completed plain text
    T(document) // create a plain-text renderer for a document
    o.build() -> String // explicitly render a document

JSON Value Patterns
===================

.. code-block:: text

    T() // create JSON null
    T(primitive-or-array-or-object) // create a JSON value
    o.type()/is(type)/isPrimitive() -> T // inspect the semantic type
    o.get(index-or-key)/getOrThrow(index-or-key) -> JsonValue // access a child value
    o.get<U>([fallback])/getOrThrow<U>() -> U // access a checked native representation
    o.set(index-or-key, value)/append(value) -> T& // detach and mutate an array or object
    o.toString([options]) -> String // serialize deterministic JSON
    T::fromString(text[, options]) -> optional<T> // parse with empty failure reporting
    T::fromStringOrThrow(text[, options]) -> T // parse or throw ParseError

Layout Renderer Patterns
========================

.. code-block:: text

    T::create([options]) -> EnvironmentPtr // create a setup-phase environment
    o.addLayoutLoader(loader[, priority])/addFilter(name, filter)/enableAutoReload() // configure before first render
    o.setGlobalContext(context) // atomically replace the global fallback snapshot
    o.render(layout[, context]) -> String // compile/cache and render a logical layout
    o.load(layout) -> optional<LayoutSource> // return one source generation or report missing
    o.contains/get/set(name[, value]) -> T // inspect or change named local values
    o.type()/isTruthy()/itemCount() -> T // inspect immutable value semantics
    o.get(index-or-name) -> Value // tolerant child lookup returning null when missing
    o.as❮Scalar❯() -> T // checked scalar access

Base-N Codec Patterns
=====================

.. code-block:: text

    T::base16/base32/base64❮Variant❯() -> base_n::BaseNFormat // create a standard format
    T(input[, format]) // create an encoder from bytes or decoder from text
    o.encode() -> AnyString // encode into the configured output width
    o.decode/decodeOrThrow() -> mem::ByteBlock // decode with empty or throwing failure reporting

Punycode and IDNA Patterns
==========================

.. code-block:: text

    T(text[, options]) // create a Punycode encoder or decoder
    o.encode() -> std::optional<String> // encode with empty failure reporting
    o.decode() -> std::optional<String> // decode with empty failure reporting
    o.encodeOrThrow() -> String // encode or throw err::ParseError
    o.decodeOrThrow() -> String // decode or throw err::ParseError
    T() // create pure RFC 3492 options without prefix, domain, normalization, or filtering policy
    T::idna2008Label() -> PunycodeOptions // create strict IDNA2008 label options
    T::idna2008Domain() -> PunycodeOptions // create strict IDNA2008 domain options
    T::network() -> PunycodeOptions // create the strict network-domain policy
