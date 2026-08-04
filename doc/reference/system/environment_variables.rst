.. index::
    single: Environment Variables

*********************
Environment Variables
*********************

Introduction
============

``EnvironmentVariables`` provides portable access to the environment of the current process.
Each instance uses the native backend for the current operating system by default.
A custom backend can be supplied for isolated tests or specialized hosts.

Environment entries are process-wide.
Changes are visible to subsequent native lookups and to child processes that inherit the environment.
Names retain the native platform's case-sensitivity rules.

Reading Variables
=================

Use ``get(name)`` when absence must be distinguished from an empty stored value.
It returns ``std::nullopt`` for a missing variable or a failed lookup.
The two-argument overload returns its explicit fallback in these cases.

``getOrThrow(name)`` returns an empty string for a stored empty value and throws ``PlatformError`` if the variable does
not exist or the native lookup fails.

Writing and Removing Variables
==============================

``set()`` and ``remove()`` report failure through their boolean result.
Their ``OrThrow`` variants preserve platform failure details in ``PlatformError``.
Removal is idempotent, so removing a variable that is already absent succeeds.
Setting an empty value stores an empty value; it does not remove the entry.

Names must not be empty or contain an equals sign or embedded null byte.
Values must not contain embedded null bytes.
The throwing methods report these portable input errors as ``ParameterError``.

Interface
=========

.. doxygenclass:: erbsland::system::EnvironmentVariables
    :members:
