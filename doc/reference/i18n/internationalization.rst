..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Internationalization; Reference
    single: Display Text
    single: Compiled Catalogs

********************
Internationalization
********************

Display texts provide the labels and format patterns used by library-generated documents.
The application owns one lazily initialized ``DisplayText`` registry, with the compiled English Core catalog as its
permanent bottom layer.
Use ``displayText(key)`` for text and ``displayFormat(key).build(...)`` for formatted output.

Compiled Catalogs
=================

Author source texts in an ELCL file using regular names and text values:

.. code-block:: text

    [Root]
    Welcome Message = "Welcome"

    [Options]
    Usage Label = "Usage"
    Help Title = "Help for {}"

The compiler normalizes these names to ``welcome_message``, ``options.usage_label``, and ``options.help_title``.
``Root`` supplies unprefixed entries.
Other named section paths supply domain prefixes; multiline text and nested named sections are supported.
Text-name keys, non-text values, section lists, and includes are rejected with source locations.
An empty catalog is valid.

Compile a catalog into a target:

.. code-block:: cmake

    erbsland_core_display_text(
        TARGET my_application
        FILE DisplayTextApp.elcl
        NAMESPACE example
    )

The default outputs are ``DisplayTextApp.hpp`` and ``DisplayTextApp.cpp`` in the calling CMake binary directory, and the
class name is ``DisplayTextApp``.
Set ``HEADER``, ``SOURCE``, or ``CLASS`` to override these defaults; omit ``NAMESPACE`` for a global class.
``FILE`` is relative to the calling source directory; relative output paths use the calling binary directory.
Output paths must stay inside the build tree.
The generated header directory is added to the target's private include directories.
C++ class and namespace names must be non-reserved identifiers.

Keep the ELCL source in version control; the macro's generated files are build artifacts.
The build runs the compiler when its inputs change and writes files only when their content changes.
Generated build artifacts omit copyright and license notices.
Cleaning removes generated files, and the next build recreates them.
Cross builds must set ``ERBSLAND_CORE_DISPLAY_TEXT_COMPILER_EXECUTABLE`` to a native host compiler.
Both CMake compiler executables link against the normal Core library.
The display-text compiler target is built only for native configurations; Core-only cross configurations do not require
a host resource compiler to embed its templates.

The standalone ``erbsland-core-display-text-compiler`` accepts ``--input``, ``--header-output``, ``--source-output``,
``--class``, and ``--namespace``.
Unlike the CMake macro, its default output paths are siblings of the input file.

Core itself compiles its checked-in base catalog directly, without running a generator during the build.
After editing its ELCL source, run ``./erbsland-core-tool generate-display-texts`` and commit the updated outputs.
See :doc:`/development/tool_usage/generate_display_texts` for the regeneration workflow.

Generated tables use permanent ``StringLiteral`` backing arrays, sorted by domain and entry name.
``DisplayTextData`` binary-searches those arrays without constructing a runtime source map.
Custom data implementations must keep domains, entries, and literal storage immutable and alive for their complete
instance lifetime.

Layers and Lookup
=================

Register a generated catalog with the application:

.. code-block:: cpp

    const auto words = example::DisplayTextApp::create();
    el::application().displayText()->addData(words);
    const auto label = el::displayText("options.usage_label"_el);
    const auto removed = el::application().displayText()->removeData(words);

Registration is unique by object identity.
Adding an existing instance leaves its order unchanged.
Removal reports whether an application layer was present; the built-in bottom layer cannot be removed.
Data pointers must be non-null.
The registry retains shared ownership until removal.

For ``domain.subdomain.name``, lookup searches every layer for the exact key, then every layer for ``domain.name``, then
every layer for ``name``.
Each search visits the newest layer first.
A lower-layer exact match therefore wins over an upper-layer generic fallback.
An empty source text is a match.
Missing keys return the original requested key.
Runtime keys must already be normalized; lookup is case-sensitive and does not normalize them.

Registry changes and lookups are serialized.
Adding or removing a layer clears translated-text and format caches.
Previously returned strings and formats remain usable; later lookups reflect the new layers.
Diagnostics and option renderers use this application registry without separate wording parameters.

Translation and Formats
=======================

Install a ``DisplayTextTranslator`` with ``setTranslator()``.
Translation receives the originally requested key and the resolved source text, including when lookup used a fallback.
An empty translation retains the source.
Missing keys do not invoke the translator.
Locale selection and translation-catalog loading belong to translator implementations.
Translator callbacks must not reenter the registry.

Setting the translator always clears both caches, including when reinstalling the same pointer.
Set a null translator to restore untranslated source text.
``format()`` compiles the resolved display text and reports ``FormatError`` for invalid format syntax.
The untranslated English catalog remains available to internal option-default comparisons so application-authored help
text is preserved.

Interface
=========

.. doxygenclass:: erbsland::i18n::DisplayText
    :members:

.. doxygenfunction:: erbsland::i18n::displayText(const text::String &key) -> text::String

.. doxygenfunction:: erbsland::i18n::displayFormat(const text::String &key) -> text::StringFormat
.. doxygenclass:: erbsland::i18n::DisplayTextData
    :members:
.. doxygenclass:: erbsland::i18n::DisplayTextTranslator
    :members:
