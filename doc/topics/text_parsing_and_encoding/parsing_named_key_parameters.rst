..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Named-Key Parameters; Parsing
    single: Named-Key Parser

************************************
Parsing Compact Named-Key Parameters
************************************

A short option field becomes easier to read when each value has a name.
Consider ``subject=revontulet,lang=fi,brief``: it names a subject and language, then adds a bare ``brief`` option.
An application could use it to request a short fact card about the northern lights in Finnish.

The named-key parser reads this kind of field and returns its entries in order.
This page shows how to give those entries meaning in your application.

Where Named-Key Blocks Fit
==========================

A named-key block is a short list of entries inside a larger piece of text.
An entry may pair a key with a value, as ``lang=fi`` does, or contain only a key, as ``brief`` does.
By default, commas separate entries and ``=`` separates a key from its value.

You encounter this pattern in :cpp:type:`StringFormat <erbsland::text::StringFormat>` specifications and in
configuration placeholders.
It fits places where a few readable options must live inside another string, without the structure of a complete
document format.

The parser can tell you which registered key it found and whether that entry carries a value.
It cannot know what those choices mean to your application.
That distinction matters when you decide which checks belong in the format and which belong in your own code.

From a Field to Application Options
===================================

First, give each option a stable identifier.
An enum makes the identifiers easier to recognize in the code that handles parsed entries.
Then register the spellings readers may write in :cpp:class:`Format <erbsland::text::named_key::Format>`.

Two spellings may share one identifier.
For example, ``language`` and ``lang`` both mean the same option, so the application does not need separate handling for
them.

To read the field, give :cpp:class:`named_key::Parser <erbsland::text::named_key::Parser>` a
:cpp:class:`StringCharReader <erbsland::text::StringCharReader>` and the format.
``readAllEntries()`` returns the entries in their original order.
Each :cpp:class:`Entry <erbsland::text::named_key::Entry>` reports its kind, value, and registered key identifier.

The following example translates the returned entries into application options.
Notice that it checks the entry kind as well as the identifier: ``subject`` and ``language`` need values, while
``brief`` is a bare switch.

.. erbsland-demo::
    :source: text/NamedKeyParsing/ParseFactRequest.cpp
    :function-blocks: parseFactRequest
    :function-blocks-sha256: 8eed815e85bd7d26d1f5babf0255d9e8117e6590376bb233e679cc63cc4eabe1
    :exec: text/named_key_parsing --demo ParseFactRequest
    :source-sha256: b7534f5fe378488a335d79217fe085beb5b8e787ad31e4f7e6854a394eb68121

.. code-block:: cpp

    void parseFactRequest() {
        enum class FactOption { Subject, Language, Brief };
        const auto format = el::named_key::Format{}
                                .setKeys({
                                    {"subject"_el, static_cast<int>(FactOption::Subject)},
                                    {"language"_el, static_cast<int>(FactOption::Language)},
                                    {"lang"_el, static_cast<int>(FactOption::Language)},
                                    {"brief"_el, static_cast<int>(FactOption::Brief)},
                                })
                                .setValueListAllowed(false);
        auto reader = el::StringCharReader{el::String{"subject=revontulet,lang=fi,brief"_el}};
        auto parser = el::named_key::Parser{reader, format};

        // Translate parsed entries into the application's options.
        auto subject = el::String{};
        auto language = el::String{};
        auto brief = false;
        auto valid = true;
        for (const auto &entry : parser.readAllEntries()) {
            switch (static_cast<FactOption>(entry.keyIndex())) {
            case FactOption::Subject:
                valid = valid && entry.isKeyWithValue();
                subject = entry.value();
                break;
            case FactOption::Language:
                valid = valid && entry.isKeyWithValue();
                language = entry.value();
                break;
            case FactOption::Brief:
                valid = valid && entry.isKey();
                brief = entry.isKey();
                break;
            }
        }
        if (!valid || subject.isEmpty() || language.isEmpty()) {
            el::io::printLine("Invalid fact request"_el);
            return;
        }
        el::io::printLine("Subject: "_el, subject);
        el::io::printLine("Language: "_el, language);
        el::io::printLine("Brief: "_el, brief);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Subject: revontulet
    Language: fi
    Brief: true

.. erbsland-demo-end::

The sample uses the ``lang`` alias, yet the switch handles it as ``Language``.
It also accepts ``brief`` without a value.
This is why the code tests both the key identifier and the entry kind before using a value.

The call to ``setValueListAllowed(false)`` makes this field a named-entry list.
Without that setting, an unrecognized bare word at the beginning could be treated as a positional value instead of an
unknown key.

Deciding What to Reject
=======================

The parser checks the syntax and the registered keys.
Your application checks rules that depend on what each key *means*, such as requiring a value for ``subject`` but a bare
switch for ``brief``.
Keeping those checks separate lets one format serve fields whose keys have different roles.

Input can also be malformed before the application examines an entry.
For input you do not control, handle :cpp:class:`ParseError <erbsland::err::ParseError>`; it includes a position for
syntax errors, unknown keys, and duplicate identifiers.
The spellings ``language`` and ``lang`` share an identifier, so ``language=fi,lang=sv`` counts as a duplicate under the
default unique-key policy.

Keeping a Vocabulary Consistent
===============================

If several fields use related options, define their identifiers and spellings together.
Reusing the format then keeps ``lang`` tied to the same meaning everywhere.
Keep that format alive while a parser uses it, because the parser holds a reference to it.

Sometimes one field accepts only part of a shared vocabulary.
Before reading, call ``setAllowedKeys()`` on its parser with the permitted semantic identifiers.
An alias follows its identifier, so restricting the field to ``Language`` permits both ``language`` and ``lang``.

When fields need different syntax or value policies, give them separate formats.
The :doc:`configuring_named_key_parameters` and :doc:`customizing_named_key_syntax` topics explain those choices; the
:doc:`/reference/text/named_keys` reference lists the complete interface.
