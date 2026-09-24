..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Placeholders; Built-in Sources
    single: Environment Variables; Configuration Placeholders
    single: Application Variables; Configuration Placeholders

**********************************
Using Built-in Placeholder Sources
**********************************

Configuration text sometimes needs values that do not belong in the document itself.
Deployment systems may provide a region or service endpoint through the process environment, while application code may
already know a display name or runtime mode.
The built-in ``env`` and ``var`` placeholder sources cover these two common cases without requiring a custom
:cpp:class:`PlaceholderSource <erbsland::conf::PlaceholderSource>`.

Both sources are deliberately opt-in.
Until you enable one of them, placeholder-looking text remains unchanged.
Use ``env`` when the process environment is the authority for a value, and use ``var`` when your application should
decide exactly which values the configuration can read.

Reading Values from the Process Environment
===========================================

Call
:cpp:func:`Parser::enableEnvironmentPlaceholderSource()
<erbsland::conf::Parser::enableEnvironmentPlaceholderSource>` before parsing to enable the ``env`` source.
The text after ``env:`` is passed unchanged to the platform environment lookup.
This makes the source a natural boundary between an ELCL document and settings supplied by a service manager, container
runtime, or deployment script.

The following demo creates a repeatable process environment, enables the source, and reads two research settings.
Its Greek values come from the generated demo theme and represent external data; the API works with the same Unicode
text on every supported platform.

.. erbsland-demo::
    :source: conf/Placeholders/BuiltInSources.cpp
    :function-blocks: builtInEnvironmentSource
    :function-blocks-sha256: 79689d7e0369354e0d59c323c29786fc43b6164c06fb8b86746298aa64de739f
    :exec: conf/placeholders --demo BuiltInEnvironmentSource
    :source-sha256: 042da33588052024a9275dc00a7e0d2e8e12304d0edebac2a0a2a2f63181e2a7

.. code-block:: cpp

    void builtInEnvironmentSource() {
        auto environment = el::system::EnvironmentVariables{};
        environment.setOrThrow("ERBSLAND_DEMO_RESEARCH_TITLE"_el, "Χαρτογράφηση ωκεάνιων ρευμάτων"_el);
        environment.setOrThrow("ERBSLAND_DEMO_RESEARCH_REGION"_el, "Αιγαίο\t Πέλαγος"_el);
        environment.removeOrThrow("ERBSLAND_DEMO_RESEARCH_MISSING"_el);

        auto parser = el::conf::Parser{};
        parser.enableEnvironmentPlaceholderSource();
        const auto document = parser.parseTextOrThrow(
            "[research]\n"
            "title: \"${env:ERBSLAND_DEMO_RESEARCH_TITLE}\"\n"
            "region: \"${env:ERBSLAND_DEMO_RESEARCH_REGION}\"\n"
            "missing: \"${env:ERBSLAND_DEMO_RESEARCH_MISSING}\"\n"_el);

        environment.removeOrThrow("ERBSLAND_DEMO_RESEARCH_TITLE"_el);
        environment.removeOrThrow("ERBSLAND_DEMO_RESEARCH_REGION"_el);

        el::io::printLine("Title: "_el, document->getTextOrThrow("research.title"_el));
        el::io::printLine("Region: "_el, document->getTextOrThrow("research.region"_el));
        el::io::printLine("Missing: "_el, document->getTextOrThrow("research.missing"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Title: Χαρτογράφηση ωκεάνιων ρευμάτων
    Region: Αιγαίο   Πέλαγος
    Missing: undefined

.. erbsland-demo-end::

In a real application, the parent process normally sets these variables before the application starts.
The calls to :cpp:class:`EnvironmentVariables <erbsland::system::EnvironmentVariables>` only make the demo independent
of the shell from which it is run.

Handling Missing and Empty Environment Values
---------------------------------------------

The source produces the fixed text ``undefined`` when a variable does not exist, its name is invalid for the platform
abstraction, or the platform lookup fails.
An existing variable whose value is empty still produces empty text, so the document can distinguish an empty value from
a missing variable.

Add the flag ``required`` when a missing variable must stop parsing:

.. code-block:: elcl

    token: "${env:APPLICATION_TOKEN,required}"

This source-level requirement checks whether the variable exists; it accepts an existing empty value.
Use the :doc:`built-in-filters` ``required`` filter as well when empty text must also be rejected.

Keeping Environment Text Safe
-----------------------------

Environment values enter the parsed document as ordinary text.
Before insertion, the source removes Unicode control and format characters except tab and newline.
This preserves structured multi-line text such as JSON while still removing terminal controls and directional formatting
by default.

The ``unsafe_raw`` flag disables this filtering completely:

.. code-block:: elcl

    payload: "${env:APPLICATION_PAYLOAD,unsafe_raw|escape:format=json,amount=required}"

Raw values can contain control and format characters.
Only use ``unsafe_raw`` when the following filters or the receiving application enforce the required character policy.
Flags can be combined, for example ``${env:APPLICATION_PAYLOAD,required,unsafe_raw}``.

This cleanup is intentionally narrow.
If a setting also needs surrounding whitespace removed, a bounded display representation, or escaping for another
syntax, chain the appropriate :doc:`built-in-filters` after the source.

Choosing Portable Environment Names
-----------------------------------

The ``env`` parameter is a platform environment-variable name, not an ELCL name.
Its spelling and case are preserved, and the operating system decides whether names are case-sensitive.
For configuration intended to run on several platforms, simple uppercase ASCII names containing letters, digits, and
underscores are the least surprising choice.

Enabling the source registers the name ``env`` on that parser.
Calling the method twice, or registering another source with the same normalized name, raises an
:cpp:class:`LogicError <erbsland::err::LogicError>` because source names must be unique.
The registration remains active for all later documents parsed with that parser.

Providing Values from the Application
=====================================

The ``var`` source turns an application-owned :cpp:type:`StringMap <erbsland::text::StringMap>` into a small, explicit
interface for configuration documents.
It is useful when a value comes from command-line processing, embedded application metadata, or another trusted part of
the program rather than from the process environment.

Call
:cpp:func:`Parser::setPlaceholderVariables() <erbsland::conf::Parser::setPlaceholderVariables>` with the complete map
before parsing.
The following demo exposes the title and featured species for an insect collection.
The ELCL input deliberately spells one parameter differently to demonstrate regular ELCL name matching.

.. erbsland-demo::
    :source: conf/Placeholders/BuiltInSources.cpp
    :function-blocks: builtInVariableSource
    :function-blocks-sha256: 3ff1bf2581d1784907dae94abcd7f31094b4a492dd56506d1b1d129488cfea52
    :exec: conf/placeholders --demo BuiltInVariableSource
    :source-sha256: 042da33588052024a9275dc00a7e0d2e8e12304d0edebac2a0a2a2f63181e2a7

.. code-block:: cpp

    void builtInVariableSource() {
        auto parser = el::conf::Parser{};
        parser.setPlaceholderVariables(
            el::text::StringMap<el::text::String>{{
                {"collection title"_el, "Σκαθάρια του Ολύμπου"_el},
                {"featured species"_el, "Carabus olympiae"_el},
            }});
        const auto document = parser.parseTextOrThrow(
            "[exhibit]\n"
            "title: \"${var:COLLECTION_TITLE}\"\n"
            "featured species: \"${var:featured species}\"\n"_el);

        el::io::printLine("Exhibit: "_el, document->getTextOrThrow("exhibit.title"_el));
        el::io::printLine("Featured species: "_el, document->getTextOrThrow("exhibit.featured species"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Exhibit: Σκαθάρια του Ολύμπου
    Featured species: Carabus olympiae

.. erbsland-demo-end::

Variable names follow regular ELCL name rules.
They are case-insensitive, and spaces and underscores are equivalent, so ``collection title``, ``COLLECTION_TITLE``, and
``Collection Title`` identify the same entry.
Invalid names and names that become duplicates after normalization are rejected when the map is set.

An unknown ``var`` name always raises a configuration error.
This strict behavior is useful for application variables because a missing entry usually means that the document and the
application no longer agree on their interface.
An entry may still contain empty text when that is meaningful to the application.

Updating the Application Interface
----------------------------------

Calling ``setPlaceholderVariables()`` again replaces the complete map.
The ``var`` source remains registered, so later documents parsed with the same parser see the new values and none of the
old entries.
Replacing the map as one unit keeps related values consistent and avoids leaving a stale variable available by accident.

Like every placeholder result, a ``var`` value is inserted into quoted ELCL text before parsing continues.
Use :doc:`built-in-filters` when the document needs to trim, escape, select, or validate that text.
When values require a lookup protocol or lifecycle beyond a fixed map, implement the custom source described in
:doc:`placeholders` instead.
