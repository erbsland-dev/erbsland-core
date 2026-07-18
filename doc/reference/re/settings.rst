********
Settings
********

The :cpp:class:`Settings <erbsland::re::Settings>` class allows you to control which features are accepted and which
limits apply when compiling and executing regular expressions.

Settings are applied during compilation and define the behaviour of the resulting
:cpp:class:`RegEx <erbsland::re::RegEx>` instance.
They are especially important when your application processes patterns from external or untrusted sources, such as user
configuration files or plug-in systems.

Limiting the Run-Time
=====================

When working with externally provided patterns, it is strongly recommended to limit the maximum run-time of matching
operations.

Using :cpp:func:`setTimeout <erbsland::re::Settings::setTimeout>`, you can define a per-call time limit for all matching
operations performed by a
:cpp:class:`RegEx <erbsland::re::RegEx>` instance.

If a matching operation exceeds the configured timeout, it is aborted and an
:cpp:class:`RegExError <erbsland::re::RegExError>` exception with the error category
``Timeout`` is thrown.

This mechanism protects your application against excessively complex patterns or pathological input that would otherwise
lead to long or unbounded execution times.

Setting Pattern Complexity Limits
=================================

In addition to time limits, the library provides a set of limits that restrict individual aspects of pattern complexity,
such as nesting depth, repetition counts, or internal resource usage.

These limits can only be *tightened*, never extended.
The library ships with a carefully chosen set of safe default limits that work well for most use cases.

Reducing these limits is useful when you want to:

*   Prevent excessive memory consumption
*   Guard against denial-of-service scenarios
*   Enforce predictable performance characteristics

All limits are checked during compilation or execution and result in a well-defined error if exceeded.

Enabling or Disabling Features
==============================

By default, the regular expression support in Erbsland Core enables a number of compatibility features to ease migration
from existing regular expression engines.

While convenient, some of these features come with ambiguities, surprising edge cases, or performance costs.
For applications that accept patterns from external sources, it is often desirable to restrict the accepted syntax more
strictly.

The feature flags in :cpp:class:`Settings <erbsland::re::Settings>` allow you to explicitly control which parts of the
pattern syntax are enabled.

A sensible starting point for untrusted patterns is disabling ``AllCompatibility``, thereby accepting only the core,
well-defined syntax.

Certain particularly problematic features are disabled by default:

*   ``EmptyAlternatives``
*   ``EmptyGroups``

Both can lead to unintuitive matches and make patterns harder to reason about.
If you enable them, you should do so consciously and only when their behaviour is clearly understood and required.

Choosing the Right Settings
===========================

For internal, fully controlled patterns, the default settings are usually sufficient and provide maximum convenience.

For externally supplied patterns, we recommend:

*   Enabling a strict timeout
*   Tightening complexity limits where possible
*   Disabling compatibility features that are not explicitly required

This layered approach keeps your application robust while still allowing powerful and expressive regular expressions.

.. button-ref:: input
    :ref-type: doc
    :color: success
    :align: center
    :expand:
    :class: sd-fs-5 sd-font-weight-bold sd-p-2 sd-my-4

    The Input Interface →


Interface
=========

.. doxygenenum:: erbsland::re::Feature

.. doxygenfunction:: erbsland::re::toString(const Feature feature) -> text::String
.. doxygenclass:: erbsland::re::Features
    :members:
.. doxygenclass:: erbsland::re::Settings
    :members:
