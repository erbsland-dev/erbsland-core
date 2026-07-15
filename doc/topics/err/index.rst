..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Errors
    single: Exceptions
    single: Diagnostics

**********************************
Exceptions, Errors and Diagnostics
**********************************

Errors become easier to handle when each layer has a clear responsibility.
These pages introduce the common failure types in Erbsland Core, show how to preserve useful context while an error
travels through your application, and explain how structured diagnostics turn technical failures into helpful messages.

.. toctree::
    :maxdepth: 2

    overview
    reporting_with_result
    handling_exceptions
    diagnostics
    writing_custom_exceptions
