..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Named-Key Parsing; Key Vocabulary
    single: Named-Key Parsing; Entry Policies

*************************************
Configuring Named-Key Parameter Lists
*************************************

A named-key field needs a clear vocabulary: which names can appear, which names mean the same thing, and which kinds of
entry are valid here?
:cpp:class:`Format <erbsland::text::named_key::Format>` records those decisions for the parser.

You will learn how to register keys, choose between named and positional entries, and set useful limits on values.
If you are new to the parser, begin with :doc:`parsing_named_key_parameters` for a complete reading example.

Give Keys Stable Meanings
=========================

A spelling is what a reader types; an identifier is what your application uses after parsing.
Keeping these separate lets you accept aliases without giving the rest of your code several names for one option.
Each registered spelling maps to a nonnegative integer identifier.

For example, ``topic`` and ``subject`` can share one identifier.
A parsed entry then has the same meaning whichever spelling appeared in the input.
The first spelling registered for an identifier is its preferred name: ``keyName()`` returns that spelling when you need
to display it.

Registering Spellings
---------------------

The key table uses normalized names such as ``topic`` and ``quiz_level``.
Readers need not match the letter case of those names: ``SUBJECT`` finds a registered ``subject`` alias.
Write the *registered* spelling in normalized form, and give every alias a unique name.

``setKeys()`` replaces the whole table when you know the vocabulary at once.
``addKey()`` appends one spelling when you are building it step by step.
Both validate new entries; for example, a second registration of the same alias is rejected even if it has a different
identifier.

Looking Up a Name
-----------------

The table also works in both directions after registration.
``keyIndex()`` returns an optional identifier for an input spelling, so you can check whether that spelling is known.
``keyName()`` takes an identifier and returns its first registered spelling; it throws if the identifier is absent.

The following example registers a preferred name and an alias, then prints parsed entries using the preferred name.
Its small vocabulary is only an illustration; the same pattern works for any compact field.

.. erbsland-demo::
    :source: text/NamedKeyParsing/RegisterQuizKeys.cpp
    :exec: text/named_key_parsing --demo RegisterQuizKeys
    :source-sha256: a20577477d0d4a44eae5cd4191f251a9cf4959746a7f7415eb87e114c0611c16

.. code-block:: cpp

    /// Register a vocabulary for a compact quiz request.
    ///
    /// `Format` stores normalized key spellings and integer identifiers. Several spellings can share one identifier,
    /// while the first registered spelling is the name returned by `keyName()`.
    void registerQuizKeys() {
        enum class QuizKey { Topic, Level };
        auto format = el::named_key::Format{}.setKeys({
            {"topic"_el, static_cast<int>(QuizKey::Topic)},
            {"subject"_el, static_cast<int>(QuizKey::Topic)},
        });
        format.addKey("level"_el, static_cast<int>(QuizKey::Level));
        format.setValueListAllowed(false);

        // The spelling in the input is normalized before lookup.
        auto reader = el::StringCharReader{el::String{"SUBJECT=gezegen,level=2"_el}};
        for (const auto &entry : el::named_key::Parser{reader, format}.readAllEntries()) {
            el::io::printLine(format.keyName(entry.keyIndex()), ": "_el, entry.value());
        }
        el::io::printLine("Registered aliases: "_el, format.keys().count());
    }

.. erbsland-ansi::
    :escape-char: ␛

    topic: gezegen
    level: 2
    Registered aliases: 3

.. erbsland-demo-end::

Although the input says ``SUBJECT``, the output says ``topic``.
The parser resolves the alias to its identifier, and ``keyName()`` turns that identifier back into the preferred
spelling.
The output also reports three registered aliases, even though they represent only two options.
``keys()`` exposes the spellings in the table, so its count is an alias count.

The example also disables positional values because this field accepts named entries only.
That choice prevents an unknown bare word at the start from becoming a positional value.
The next section explains when to make that choice.

Choose the Forms a Field Accepts
================================

Once the keys are known, decide what a valid entry looks like in this particular field.
Does a key need a value? Can the same option appear twice? Could the field instead be a list of unnamed values?

By default, a format accepts bare keys, keyed values, and positional values.
It allows each semantic key once.
The four settings below change those rules independently, so the key vocabulary can stay the same while different fields
accept different forms.

Repeated keys
-------------

Most fields should reject a repeated option, and that is the default.
``setUniqueKeysRequired(false)`` is useful when repetition has a clear meaning, such as requesting several topics with
``topic=gezegen,topic=yıldız``.

Uniqueness follows the *identifier*, not the spelling.
If ``topic`` and ``subject`` are aliases, ``topic=gezegen,subject=yıldız`` is also a repeat.
When you allow this, collect the entries or define how your application chooses among them; otherwise a later value
could silently compete with an earlier one.

Bare keys
---------

A bare key can act like a switch: ``hint`` may be enough to request a hint.
If *every* recognized key in the field needs a value, ``setKeysWithoutValuesAllowed(false)`` makes a bare key a parse
error.
For example, it accepts ``topic=gezegen`` but rejects ``topic``.

This is a field-wide rule.
When only some keys need values, leave bare keys allowed and check the returned entry kind for each key in your
application, as the :doc:`parsing_named_key_parameters` example does.

Keyed values
------------

The opposite field shape is a set of switches.
``setValuesAllowed(false)`` lets it accept a bare ``hint`` while rejecting ``hint=on``.

This rule affects values attached to *recognized keys*.
It does not disable positional values.
For a field containing only recognized switches, disable positional values separately.

Positional values
-----------------

A positional list gives values without keys, as in ``Mars,Venüs``.
Keep that form enabled when the position or order of each value is meaningful.
For a field that requires names, ``setValueListAllowed(false)`` turns an unrecognized bare entry into an error.

The first entry determines how the rest of the list is read.
After a recognized key, an unknown key is an error.
After a positional value, later entries stay positional; ``Mars,topic=gezegen`` is two positional values rather than a
mixture of two entry kinds.
Choose separate field grammars if such an input would surprise your readers.

The following demo starts from a format that requires named entries.
It changes one policy at a time, then explicitly enables positional values for its answer-list variant.

.. erbsland-demo::
    :source: text/NamedKeyParsing/ChooseEntryPolicies.cpp
    :exec: text/named_key_parsing --demo ChooseEntryPolicies
    :source-sha256: 0c827669e1ae252a196e5ae6823a3c4e76f794821db623240468efcd57037978

.. code-block:: cpp

    /// Choose which forms of quiz options a field accepts.
    ///
    /// The entry-form switches are independent: repeated keys, bare keys, keyed values, and positional values can
    /// each be enabled or disabled according to the field's grammar.
    void chooseEntryPolicies() {
        const auto base = el::named_key::Format{}.setKeys({{"topic"_el, 0}, {"hint"_el, 1}}).setValueListAllowed(false);

        // A repeated semantic key can represent several requested topics.
        const auto repeated = el::named_key::Format{base}.setUniqueKeysRequired(false).setValueListAllowed(false);
        auto repeatedReader = el::StringCharReader{el::String{"topic=gezegen,topic=yıldız"_el}};
        el::io::printLine("Topics: "_el, el::named_key::Parser{repeatedReader, repeated}.readAllEntries().count());

        // Require a value after each recognized key.
        const auto valued = el::named_key::Format{base}.setKeysWithoutValuesAllowed(false);
        auto valuedReader = el::StringCharReader{el::String{"topic=gezegen"_el}};
        el::io::printLine("Valued key: "_el, el::named_key::Parser{valuedReader, valued}.readEntry().isKeyWithValue());

        // A flags-only field accepts a bare key but no keyed value.
        const auto flags = el::named_key::Format{base}.setValuesAllowed(false);
        auto flagReader = el::StringCharReader{el::String{"hint"_el}};
        el::io::printLine("Bare flag: "_el, el::named_key::Parser{flagReader, flags}.readEntry().isKey());

        // A positional-answer field accepts a value without a registered key.
        const auto answers = el::named_key::Format{base}.setValueListAllowed(true);
        auto answerReader = el::StringCharReader{el::String{"gezegen"_el}};
        el::io::printLine("Positional answer: "_el, el::named_key::Parser{answerReader, answers}.readEntry().isValue());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Topics: 2
    Valued key: true
    Bare flag: true
    Positional answer: true

.. erbsland-demo-end::

Bound Positional Lists and Value Length
=======================================

The entry-form policies decide what a value looks like.
Limits decide how much of it you will accept.
They are especially useful when a short field comes from input you do not control.

Counting Positional Entries
---------------------------

``setMaximumValues()`` caps the number of entries in a positional list.
Its default is unlimited.
Named entries do not count toward this limit, because they belong to the key-based form of the field.

Limiting Each Value
-------------------

``setMaximumValueLength()`` applies to every value, whether it follows a key or appears on its own.
It counts Unicode code points, rather than the number of encoded bytes.
The default limit is 200 code points; key names are not counted as values.

The example below permits two short positional answers.
It also shows that a too-long value is rejected when attached to a registered key.

.. erbsland-demo::
    :source: text/NamedKeyParsing/LimitAnswerValues.cpp
    :exec: text/named_key_parsing --demo LimitAnswerValues
    :source-sha256: 4c68e0c047b320041059aee593225b3b0f56b5d65a1f305a86d6d53dbb373688

.. code-block:: cpp

    /// Bound the number and length of positional quiz answers.
    ///
    /// `setMaximumValues()` counts positional entries, while `setMaximumValueLength()` measures the decoded value of
    /// every positional or keyed entry in Unicode code points.
    void limitAnswerValues() {
        const auto format =
            el::named_key::Format{}.setMaximumValues(el::unit::ItemCount{2U}).setMaximumValueLength(el::unit::CpLength{5U});
        auto acceptedReader = el::StringCharReader{el::String{"Mars,Venüs"_el}};
        el::io::printLine("Accepted answers: "_el, el::named_key::Parser{acceptedReader, format}.readAllEntries().count());

        // A third positional entry exceeds the count limit.
        auto extraReader = el::StringCharReader{el::String{"Mars,Venüs,Ay"_el}};
        try {
            const auto entries = el::named_key::Parser{extraReader, format}.readAllEntries();
            el::io::printLine("Unexpected accepted answers: "_el, entries.count());
        } catch (const el::err::ParseError &) {
            el::io::printLine("Third answer rejected"_el);
        }

        // The length limit also applies to a value attached to a key.
        auto keyedFormat = format;
        keyedFormat.addKey("topic"_el, 0);
        auto longReader = el::StringCharReader{el::String{"topic=gezegen"_el}};
        try {
            const auto entry = el::named_key::Parser{longReader, keyedFormat}.readEntry();
            el::io::printLine("Unexpected accepted topic: "_el, entry.value());
        } catch (const el::err::ParseError &) {
            el::io::printLine("Long topic rejected"_el);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Accepted answers: 2
    Third answer rejected
    Long topic rejected

.. erbsland-demo-end::

The third answer exceeds the count limit.
The keyed ``topic`` value exceeds the length limit.
Both failures become :cpp:class:`ParseError <erbsland::err::ParseError>` with an input position, so a caller can explain
where the field went wrong.

Choose limits that fit the surrounding document or protocol.
That way the field accepts useful input while still bounding work on unexpected input.

When the field's punctuation or compact spelling differs from the defaults, continue with
:doc:`customizing_named_key_syntax`.
