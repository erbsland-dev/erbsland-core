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

Persistent Secrets
==================

:cpp:class:`SecureStorageSource <erbsland::text::placeholder::SecureStorageSource>` resolves ``${secure:item}`` through a namespace-bound ``cryptology::SecureStorage``.
Register it explicitly through ``Replacer::addSource()`` or ``conf::Parser::addPlaceholderSource()``.
It uses current-user Keychain items on macOS and Credential Manager items on Windows; Linux reports unsupported.
See :doc:`/reference/cryptology/protected_data` for storage limits and interaction policy.

Validation checks the item name without accessing storage.
Resolution retrieves the value synchronously and preserves its exact UTF-8 text, including whitespace and empty values.
Invalid UTF-8 and control characters other than tab, line feed, and carriage return are rejected.
Missing items report ``ReplacerErrorCategory::ValueNotFound``; storage access failures report ``Access``.
Diagnostics do not include the retrieved value.

Resolved text uses sensitive UTF-8 storage, which propagates through replacement, built-in filters, and concatenation.
This does not set ``conf::Value::isSecret()``: applications must annotate secret fields in validation rules and control
logging and export.
Custom filters remain responsible for plaintext copies they create.
See :doc:`/topics/cryptology/configuring_tls` for registering the source and loading a signing-key password.

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
.. doxygenclass:: erbsland::text::placeholder::SecureStorageSource
    :members:
.. doxygenclass:: erbsland::text::placeholder::Source
    :members:

.. doxygentypedef:: erbsland::text::placeholder::SourcePtr
.. doxygenclass:: erbsland::text::placeholder::TextFilter
    :members:
.. doxygenclass:: erbsland::text::placeholder::VariableSource
    :members:
