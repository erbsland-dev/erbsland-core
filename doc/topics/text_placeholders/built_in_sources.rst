..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Placeholders; Built-in Sources
    single: Environment Variables; Text Placeholders
    single: Application Variables; Text Placeholders

**********************************
Using Built-in Placeholder Sources
**********************************

A placeholder needs a source before it can produce text.
Sometimes the value belongs to the application, such as the name of a field site.
At other times the process environment supplies it, such as a setting from the program's launcher.
The built-in ``var`` and ``env`` sources cover these cases without a custom
:cpp:class:`Source <erbsland::text::placeholder::Source>`.

Neither source is active on a new :cpp:class:`Replacer <erbsland::text::placeholder::Replacer>`.
Register the sources whose values your text may read, then call ``replaceOrThrow()`` for a completed result.
The same expressions can also appear in quoted ELCL text; :doc:`/topics/conf/placeholders` explains when its parser
expands them.

Reading the Process Environment
===============================

Call ``addEnvironmentSource()`` to register ``env``.
The parameter after ``env:`` is a platform environment-variable name whose spelling and case are preserved.
The source reads its value when replacement runs, so later calls can see changes to the process environment.

The demo sets a variable only to make its output repeatable.
An application would normally receive that variable from its parent process.

.. erbsland-demo::
    :source: text/Placeholders/BuiltInSources.cpp
    :function-blocks: readEnvironmentSource
    :function-blocks-sha256: 138cd0409dc7a74f6f7e9778248b73c5e049e6f4918aea4d9247d39bc49955b6
    :exec: text/text_placeholders --demo ReadEnvironmentSource
    :source-sha256: 8bd2fb2e8344b19376d5802fe2fbc159a228980225acbe34ce483f004f28d413

.. code-block:: cpp

    void readEnvironmentSource() {
        auto environment = el::system::EnvironmentVariables{};
        environment.setOrThrow("ERBSLAND_DEMO_VOLCANO_REGION"_el, u8"  Vulkanområdet\tAskja  "_el);

        auto replacer = el::placeholder::Replacer{};
        replacer.addEnvironmentSource();
        replacer.addTextFilters();
        el::io::printLine(replacer.replaceOrThrow("Region: ${env:ERBSLAND_DEMO_VOLCANO_REGION|required|trim}"_el));

        environment.removeOrThrow("ERBSLAND_DEMO_VOLCANO_REGION"_el);
        el::io::printLine(replacer.replaceOrThrow("Unknown: ${env:ERBSLAND_DEMO_VOLCANO_REGION}"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Region: Vulkanområdet   Askja
    Unknown: undefined

.. erbsland-demo-end::

The ``,required`` source flag reports a missing variable as a
:cpp:class:`ReplacerError <erbsland::text::placeholder::ReplacerError>` with the ``ValueNotFound`` category.
Without that flag, a missing or unreadable variable produces the literal text ``undefined``.
An existing variable containing an empty string produces empty text; add the :doc:`built_in_filters` ``required`` filter
when an empty value is also unacceptable.

Environment values are ordinary replacement text.
By default, ``env`` removes Unicode control and format characters except tab and newline.
This keeps line breaks in multi-line data while excluding terminal controls and directional formatting.
The ``,unsafe_raw`` flag disables that cleanup, so use it only when the destination can safely handle the raw value.
Flags can be combined, as in ``${env:APPLICATION_PAYLOAD,required,unsafe_raw}``.

For portable settings, choose simple uppercase ASCII names containing letters, digits, and underscores.
The operating system determines whether environment names are case-sensitive.
You may register the source under another name with ``addEnvironmentSource("process"_el)``; the expression then begins
with ``${process:...}``.
Registering a second source under an occupied name raises :cpp:class:`LogicError <erbsland::err::LogicError>`.

Supplying Application Values
============================

``setVariableSource()`` registers ``var`` with an application-owned
:cpp:type:`StringMap <erbsland::text::StringMap>`.
This is useful for values already known to your program, such as command-line choices or metadata for a document.
The map makes the available names explicit and keeps each request independent of the process environment.

.. erbsland-demo::
    :source: text/Placeholders/BuiltInSources.cpp
    :function-blocks: readVariableSource
    :function-blocks-sha256: deaba99f045b50c9fdd81dcdff11c8d301807e45b9604dcbec38fa8f75a583c2
    :exec: text/text_placeholders --demo ReadVariableSource
    :source-sha256: 8bd2fb2e8344b19376d5802fe2fbc159a228980225acbe34ce483f004f28d413

.. code-block:: cpp

    void readVariableSource() {
        auto replacer = el::placeholder::Replacer{};
        replacer.setVariableSource({{{"field site"_el, u8"Askja"_el}, {"rock type"_el, u8"Basalt"_el}}});
        el::io::printLine(replacer.replaceOrThrow("Site: ${var:FIELD_SITE}; rock: ${var:rock type}"_el));

        replacer.setVariableSource({{{"field site"_el, u8"Hekla"_el}}});
        el::io::printLine(replacer.replaceOrThrow("Next site: ${var:field site}"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Site: Askja; rock: Basalt
    Next site: Hekla

.. erbsland-demo-end::

Variable names follow regular ELCL name matching: names are case-insensitive, and spaces and underscores are equivalent.
Thus ``field site``, ``FIELD_SITE``, and ``Field Site`` select the same entry.
Invalid names and duplicates after normalization are rejected when the map is set.
An unknown variable produces a ``ValueNotFound`` error during strict replacement; an entry with empty text is valid.

Calling ``setVariableSource()`` again with the same source name replaces the complete map.
The second field note in the demo sees the new site; the old ``rock type`` entry is no longer available.
You may also give the source a custom name with the optional second argument.
When values need a lookup protocol rather than a fixed map, :doc:`extend_placeholders` shows how to write a provider.
