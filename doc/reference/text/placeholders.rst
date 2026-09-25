..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Text Placeholders; Reference

*****************
Text Placeholders
*****************

:cpp:class:`Replacer <erbsland::text::placeholder::Replacer>` expands placeholders in ordinary strings using
registered sources and filters.
:cpp:class:`ReplacerOptions <erbsland::text::placeholder::ReplacerOptions>` selects the frame, separators, and escape
mode.
Providers can be custom implementations of
:cpp:class:`Source <erbsland::text::placeholder::Source>` and
:cpp:class:`Filter <erbsland::text::placeholder::Filter>`, or the built-in environment, variable, and text providers.

Use :doc:`/topics/text_placeholders/placeholders` for the replacement workflow,
:doc:`/topics/text_placeholders/built_in_sources` for built-in value sources,
:doc:`/topics/text_placeholders/built_in_filters` for reusable text filters,
:doc:`/topics/text_placeholders/extend_placeholders` for custom providers, and
:doc:`/topics/text_placeholders/customize_placeholder_syntax` for syntax and escaping.
:cpp:class:`ReplacerError <erbsland::text::placeholder::ReplacerError>` reports a category and optional input offset.

Interface
=========

.. doxygenclass:: erbsland::text::placeholder::EnvironmentSource
    :members:
.. doxygenenum:: erbsland::text::placeholder::EscapeMode
.. doxygenclass:: erbsland::text::placeholder::Filter
    :members:

.. doxygentypedef:: erbsland::text::placeholder::FilterPtr
.. doxygenclass:: erbsland::text::placeholder::Replacer
    :members:

.. doxygentypedef:: erbsland::text::placeholder::ReplacerPtr
.. doxygenclass:: erbsland::text::placeholder::ReplacerError
    :members:
.. doxygenenum:: erbsland::text::placeholder::ReplacerErrorCategory
.. doxygenclass:: erbsland::text::placeholder::ReplacerOptions
    :members:
.. doxygenclass:: erbsland::text::placeholder::Source
    :members:

.. doxygentypedef:: erbsland::text::placeholder::SourcePtr
.. doxygenclass:: erbsland::text::placeholder::TextFilter
    :members:
.. doxygenclass:: erbsland::text::placeholder::VariableSource
    :members:
