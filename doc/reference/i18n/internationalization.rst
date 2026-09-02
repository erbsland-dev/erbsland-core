..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Internationalization; Reference
    single: Display Text

********************
Internationalization
********************

Display-text maps provide the English labels and format patterns used by library-generated documents.
Keys use ``domain.subdomain.name`` form and fall back to ``domain.name`` and then ``name``.
Applications can clone the default map, edit the clone, and publish it through
:cpp:func:`Application::setDisplayTextMap() <erbsland::core::Application::setDisplayTextMap>`.

A translator can replace resolved source text without changing map entries.
Translation catalog and locale selection are intentionally left to translator implementations.

Interface
=========

.. doxygenclass:: erbsland::i18n::DisplayTextMap
    :members:
.. doxygenclass:: erbsland::i18n::DisplayTextTranslator
    :members:
