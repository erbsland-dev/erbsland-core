..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Named-Key Parsing; Syntax
    single: Named-Key Parsing; Delimiters

****************************
Customizing Named-Key Syntax
****************************

A named-key field does not have to use commas and ``=``.
It may live inside a larger string whose punctuation is already defined, or offer shorter spellings for common options.

:cpp:class:`Format <erbsland::text::named_key::Format>` describes that syntax before
:cpp:class:`Parser <erbsland::text::named_key::Parser>` reads the field.
This page explains the separators, an optional stopping point, key prefixes, compact values, and value characters.
If you have not chosen the field's keys and entry forms yet, start with :doc:`configuring_named_key_parameters`.

Choose Delimiters for the Surrounding Text
==========================================

Imagine the reader starting just after an opening bracket: ``topic:gezegen;level:2]next field``.
The semicolon divides entries, the colon divides each key from its value, and ``]`` tells the parser where this field
ends.
The reader can then continue with ``next field``.

The ordinary syntax uses a comma and ``=`` instead, and ends with the reader's data.
You only need to change these characters when the surrounding text calls for a different grammar.

List separator
--------------

``setListSeparator()`` chooses the character between complete entries.
For the bracketed field above, that character is ``;``.

Because the separator marks an entry boundary, it cannot also appear as ordinary data in a value.
The parser has no quoting or escaping layer here.
It also rejects an empty entry, including one after a trailing separator.

Value separator
---------------

``setValueSeparator()`` marks the boundary between a recognized key and its value.
Here it is ``:`` rather than the default ``=``.

Changing this character does not change which keys are known.
``unknown:gezegen`` still has an unknown key, and ``topic:`` still has a missing value.
Both are errors rather than alternative forms of a valid entry.

Stop character
--------------

``setStopCharacter()`` is useful when the list is only part of the reader's data.
For the bracketed field, set it to ``]``.
The parser consumes that character and leaves the reader positioned at ``next field``.

A configured stop character is required, not merely accepted if present.
If the input ends before ``]``, parsing fails.
The default ``Char::endOfData()`` instead ends the list at the end of the input.
In either case, keep the stop character distinct from both separators.

The following demo uses all three delimiter settings together.

.. erbsland-demo::
    :source: text/NamedKeyParsing/ChooseSeparators.cpp
    :exec: text/named_key_parsing --demo ChooseSeparators
    :sha256: e56605e06ca177bf98a6762a42dc9eb2b727089c2e4945683d85ab7aec356d13

.. code-block:: cpp

    /// Read a quiz field embedded in a larger text record.
    ///
    /// The list separator divides entries, the value separator divides a key from its value, and the stop character
    /// ends this field while leaving the remaining text available to the caller.
    void chooseSeparators() {
        const auto format = el::named_key::Format{}
                                .setKeys({{"topic"_el, 0}, {"level"_el, 1}})
                                .setListSeparator(U';')
                                .setValueSeparator(U':')
                                .setStopCharacter(U']')
                                .setValueListAllowed(false);
        auto reader = el::StringCharReader{"topic:gezegen;level:2]next field"_el};
        const auto entries = el::named_key::Parser{reader, format}.readAllEntries();
        for (const auto &entry : entries) {
            el::io::printLine(format.keyName(entry.keyIndex()), ": "_el, entry.value());
        }
        el::io::printLine("Next field remains: "_el, reader.peek() == U'n');
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mtopic: gezegen
        level: 2
        Next field remains: true␛[0m

.. erbsland-demo-end::

Add Prefixes and Compact Values
===============================

Some fields benefit from short forms such as ``+hint`` or ``level3``.
The sign can qualify a key, while the digit can begin a value attached directly to it.
These are syntax choices: the key still has the same semantic identifier.

Key prefixes
------------

``setAllowedKeyPrefixes()`` supplies characters that may appear before a recognized key.
With ``+`` and ``-`` in that set, ``+hint`` and ``-hint`` both resolve to ``hint``.

The returned entry keeps the prefix separate through ``Entry::prefix()``.
Your application decides whether ``+`` means enable, include, or something else.
Without a configured prefix, a key must start immediately.
Prefix characters must not conflict with delimiters or compact-value characters.

Compact values
--------------

``setValueWithoutKeySeparatorChars()`` supplies characters that may start a value directly after a key.
Put digits in that set and ``level3`` becomes key ``level`` with value ``3``.

Only the *first* character of the compact value needs to be in this set.
The rest is read as a value and checked by the usual value rules.
Choose markers that cannot occur inside registered key names; otherwise the boundary between key and value would be
ambiguous.

The following demo combines a signed key with a compact numeric value.

.. erbsland-demo::
    :source: text/NamedKeyParsing/PrefixAndCompactValues.cpp
    :exec: text/named_key_parsing --demo PrefixAndCompactValues
    :sha256: 9513bf3cec516e50dd1711d3f69333f3ae8132448a1a724c81925ea80f53459a

.. code-block:: cpp

    /// Add optional signs and compact numeric values to a quiz field.
    ///
    /// Prefixes are reported separately from the semantic key. A compact-value character marks the start of a value
    /// without requiring the normal key/value separator.
    void prefixAndCompactValues() {
        const auto format = el::named_key::Format{}
                                .setKeys({{"hint"_el, 0}, {"level"_el, 1}})
                                .setAllowedKeyPrefixes(el::CharSet{U'+', U'-'})
                                .setValueWithoutKeySeparatorChars(el::CharSet::from(el::AsciiCategory::Digit))
                                .setValueListAllowed(false);
        auto reader = el::StringCharReader{"+hint,level3"_el};
        const auto entries = el::named_key::Parser{reader, format}.readAllEntries();
        el::io::printLine("Hint enabled: "_el, entries.get(el::unit::ItemIndex{0U}).prefix() == U'+');
        el::io::printLine("Quiz level: "_el, entries.get(el::unit::ItemIndex{1U}).value());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mHint enabled: true
        Quiz level: 3␛[0m

.. erbsland-demo-end::

Restrict the Value Alphabet
===========================

Sometimes a value is a code rather than free text.
For example, an answer code might allow ASCII letters and digits while excluding spaces and localized spelling.
``setAllowedValueChars()`` defines that alphabet for both keyed and positional values.

An empty :cpp:class:`CharSet <erbsland::text::CharSet>`, the default, adds no alphabet restriction.
The parser still rejects unsafe input characters and applies the configured value length limit.
This setting does not restrict registered key names or a prefix before the key.

.. erbsland-demo::
    :source: text/NamedKeyParsing/RestrictValueCharacters.cpp
    :exec: text/named_key_parsing --demo RestrictValueCharacters
    :sha256: 00498dff98f124f151c2afd450a8f26fc7f5112fb3b66afecbba63b7c5b44a2a

.. code-block:: cpp

    /// Limit quiz answer codes to a defined character set.
    ///
    /// `setAllowedValueChars()` checks every character in a value, after the parser has selected a keyed or positional
    /// entry. An empty set leaves values unrestricted by this particular rule.
    void restrictValueCharacters() {
        const auto format = el::named_key::Format{}
                                .setKeys({{"answer"_el, 0}})
                                .setAllowedValueChars(el::CharSet::fromPattern("a-z0-9"_el))
                                .setValueListAllowed(false);
        auto acceptedReader = el::StringCharReader{"answer=gezegen2"_el};
        el::io::printLine("Answer: "_el, el::named_key::Parser{acceptedReader, format}.readEntry().value());

        // A localized answer with a non-ASCII letter needs a broader allowed set.
        auto rejectedReader = el::StringCharReader{"answer=güneş"_el};
        try {
            const auto entry = el::named_key::Parser{rejectedReader, format}.readEntry();
            el::io::printLine("Unexpected accepted answer: "_el, entry.value());
        } catch (const el::err::ParseError &) {
            el::io::printLine("Localized answer rejected"_el);
        }
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mAnswer: gezegen2
        Localized answer rejected␛[0m

.. erbsland-demo-end::

The example accepts ``gezegen2`` but rejects ``güneş`` because ``ü`` and ``ş`` are outside its ASCII alphabet.
For localized prose, leave the set empty or define a broader one that reflects the actual input language.

When Syntax Choices Conflict
============================

Each syntax character needs a clear role.
If a key contains a compact-value marker, or a prefix conflicts with a separator, the parser cannot reliably tell where
one part of an entry ends and another begins.

``Format::validate()`` checks these relationships when you construct a parser.
An invalid *format* raises :cpp:class:`LogicFailure <erbsland::err::LogicFailure>`.
Malformed *input* instead raises :cpp:class:`ParseError <erbsland::err::ParseError>` while the parser reads it.
