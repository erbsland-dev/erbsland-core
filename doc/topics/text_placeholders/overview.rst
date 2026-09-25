..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Text Placeholders; Overview

**************************
Text Placeholders Overview
**************************

When text contains named values supplied at runtime, a replacer connects each placeholder to a source and applies any
requested filters.
These topics show how to start with the built-in providers, then adapt the workflow to your application.

:doc:`placeholders` introduces replacement, syntax, and error handling.
:doc:`built_in_sources` explains how the built-in ``var`` and ``env`` sources obtain values.
:doc:`built_in_filters` covers filters that clean, select, escape, and validate those values.

When values come from an application service or need a domain-specific transformation, :doc:`extend_placeholders` shows
how to write and register providers.
:doc:`customize_placeholder_syntax` explains frames, separators, escaping, and safe use in layered text formats.
