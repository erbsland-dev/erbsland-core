..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Configuration; Overview

**********************
Configuration Overview
**********************

The configuration parser reads standard ELCL documents into typed value trees.
These topics begin with the complete loading workflow, continue through tree navigation, typed value access, and
validation, then cover optional placeholder expansion and signed production configurations.

Parsing and Validation
======================

:doc:`parsing-documents` follows a configuration from source text to validated application settings.
It explains throwing and non-throwing parser entry points, typed value access, document lifetime, manual validation,
rule documents, and validation rules built in C++.

Value-Tree Navigation
=====================

:doc:`value-trees` explains how ELCL sections, assignments, and lists become a tree of values.
It covers name paths, local and absolute names, optional and required child lookup, parent links, and list iteration.

Typed Value Access
==================

:doc:`individual-values` explains how to turn tree nodes into native values.
It compares direct conversion with the recommended typed getters, covers strict and relaxed list views, introduces
type-checked lists and matrices, and shows how to inspect a value's type.

Validation Rules
================

:doc:`validation-rules` shows how to validate a complete document before application code reads it.
It demonstrates an ELCL rule schema compiled into the executable as a resource and the equivalent C++ workflow with
``RulesBuilder``.

Validated Values
================

:doc:`validated-documents` explains the metadata available after successful validation.
It shows how to read titles and descriptions from the matching rule, handle secret values, distinguish inserted
defaults, and recognize deliberately unvalidated branches.

Source Resolving
================

:doc:`source-resolving` follows an ``@include`` descriptor through source resolution.
It demonstrates relative paths and wildcards, shows how to restrict ``FileSourceResolver``, and implements a custom
resolver for an application-owned include scheme.

Source Access Control
=====================

:doc:`access-checks` explains how every source is checked before the parser opens it.
It covers the default filesystem boundary, all ``FileAccessCheck`` features, application rules layered on that default,
and complete custom policies such as an exact source allowlist.

Placeholder Expansion
=====================

:doc:`placeholders` explains how a quoted text can request a value from a registered source and pass that value through
filters.
Expansion is disabled by default, and applications decide which sources and filters are available.

Built-in Placeholder Sources
============================

:doc:`built-in-sources` explains how to opt into the ``env`` placeholder source for process environment values and the
``var`` source for application-owned values.
It covers missing values, safe environment text, portable names, and replacement of the application variable map.

Built-in Placeholder Filters
============================

:doc:`built-in-filters` shows how to clean, select, escape, choose, and validate placeholder text.
It includes a complete parameter reference for every built-in filter, with compiled demos for ordinary transformations
and validation failures.

Document Signatures
===================

:doc:`document-signatures` explains why configuration signatures are useful and why an independent trust anchor is
essential when only selected people may approve a configuration.
It introduces the ELCL ``@signature`` meta-value and the roles of the parser, validator, signer callback, and signing
tool before the practical validation and signing workflows.

Signature Validation
====================

:doc:`validating-document-signatures` implements a signature validator, installs it in the parser, and demonstrates
successful validation, rejection after a document change, and an explicit policy for unsigned development files.
It also explains how strict validation applies to included documents and where signer authorization belongs.

Signing Configuration Files
===========================

:doc:`signing-configuration-documents` implements a signing backend and follows a configuration through parsing,
signing, and final verification.
It explains the byte-level signing boundary and compares application-integrated, separate local, and web-based signing
workflows.
