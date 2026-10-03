.. index::
    single: C++ Tokenizer
    single: CppToken

*************
C++ Tokenizer
*************

``el::cpp::CppTokenizer`` scans C++ source for lightweight source inspection.
It belongs to ``erbsland::text::cpp`` and is available through ``<erbsland/cpp/CppTokenizer.hpp>``.
It shares immutable source storage and produces one token at a time.

Scanning
========

Construct a tokenizer from a ``String`` and call ``next()`` until ``isEnd()`` is true.
Alternatively, iterate ``CppTokenizer::tokens(source)``; the lazy generator omits the end token and retains the source
until iteration finishes.
Tokens retain their source slices independently of the tokenizer, generator, and original string object.
Copies of a tokenizer share source storage but maintain independent cursors.

.. code-block:: cpp

    using namespace el::text::literals;
    for (const auto &token : el::cpp::CppTokenizer::tokens("class Example {};"_el)) {
        if (token.kind() == el::cpp::CppToken::Kind::Identifier) {
            inspectIdentifier(token.text(), token.characterPosition());
        }
    }

``CppToken`` exposes its category, original text, and zero-based source positions through accessors.
``characterPosition()`` returns the decoded code-point position for diagnostics.
``position()`` returns the native UTF-8 byte index for slicing or matching tokens to source ranges without rescanning
preceding characters.
Both positions are relative to the input passed to the tokenizer, even when that input is itself a slice.
Whitespace is skipped.
Punctuation uses the longest supported spelling, so ``>>=`` is one token while ``..x`` becomes ``.``, ``.``, and ``x``.
Comments, literals, and preprocessing directives include their delimiters; logical-line tokens include the terminating
newline when present.
End tokens have empty text and report the source's decoded length and byte length in the respective position accessors.
A default-constructed token is an end token without either position.
``characterRange()`` and ``range()`` return the complete decoded and native source ranges.
The tokenizer records each range from the reader's start and end cursors, so obtaining a token's code-point length via
``token.characterLength()`` requires no decoding pass.
``length()`` returns the byte length; both length accessors read the stored range lengths in constant time.
Both ranges include the token delimiters but exclude whitespace skipped before the token.
End-token ranges are empty at end of input; default-token ranges are ``noRange()``.
Manually constructed tokens supply both ranges alongside the corresponding source slice.

Scope and Errors
================

The scanner recognizes identifiers including keywords, numeric tokens, punctuation, quoted and raw literals, comments,
and line-start preprocessing directives.
It retains backslash continuations inside directives and line comments.
It does not expand macros, select conditional branches, parse declarations, or validate C++ syntax.
Non-ASCII characters are accepted as identifier characters without checking the C++ identifier character tables.
Universal character names, digraphs, and general preprocessing line splicing are not interpreted.
Numeric tokens begin with an ASCII digit; for example, ``.5`` is punctuation followed by a number.
Numeric values and suffixes are not validated, and a user-defined literal suffix is returned separately.

Malformed quoted or raw literals and unterminated block comments raise ``erbsland::err::ParseError`` with the decoded
code-point position at which scanning failed.
Generator errors occur during iteration.
Recovery after an error is not supported; construct a new tokenizer for another scan.

Interface
=========

.. doxygenclass:: erbsland::text::cpp::CppToken
    :members:
.. doxygenclass:: erbsland::text::cpp::CppTokenizer
    :members:
