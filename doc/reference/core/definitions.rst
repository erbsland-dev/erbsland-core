.. index::
    single: Definitions

***********
Definitions
***********

The ``Definitions`` header provides platform and compiler detection macros used throughout the library.
These macros enable conditional compilation for different operating systems, architectures, and compilers.

Platform Detection
==================

The following macros are defined automatically:

``ERBSLAND_OS_WINDOWS``
    Defined on Windows platforms (``_WIN32`` or ``_WIN64``).

``ERBSLAND_OS_MACOS``
    Defined on Apple platforms.

``ERBSLAND_OS_UNIX``
    Defined on all other Unix-like platforms.

Short namespace aliases ``EL_OS_WINDOWS``, ``EL_OS_MACOS``, and ``EL_OS_UNIX`` are also provided when the short
namespace is enabled.

Architecture Detection
======================

``ERBSLAND_ARCH_64``
    Defined on 64-bit architectures (``_WIN64``, ``__x86_64__``, or ``__ppc64__``).

``ERBSLAND_ARCH_32``
    Defined on 32-bit architectures.

Compiler Detection
==================

``ERBSLAND_COMPILER_CLANG``
    Defined when compiling with Clang.

``ERBSLAND_COMPILER_GCC``
    Defined when compiling with GCC or compatible compilers.

``ERBSLAND_COMPILER_MSVC``
    Defined when compiling with Microsoft Visual C++.

wchar_t Size
============

``ERBSLAND_WCHAR_16BIT``
    Defined on Windows, where ``wchar_t`` is 16 bits.

``ERBSLAND_WCHAR_32BIT``
    Defined on all other platforms, where ``wchar_t`` is 32 bits.

Short Namespace
===============

The header also sets up the ``el`` short namespace alias for ``erbsland``.
This can be disabled with ``ERBSLAND_NO_SHORT_NAMESPACE`` or customized with ``ERBSLAND_SHORT_NAMESPACE`` to use a
different namespace name.

Interface
=========

.. doxygenfile:: Definitions.hpp
