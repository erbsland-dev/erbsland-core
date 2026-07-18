
.. index::
    single: Input

*******************
The Input Interface
*******************

The :cpp:class:`Input <erbsland::re::Input>` interface allows you to provide custom input sources to the regular
expression engine.

Because the regular expression engine in Erbsland Core is based on a Thompson NFA, input is consumed sequentially and
processed in a highly efficient streaming fashion.
This makes it possible to match patterns not only against built-in Core strings, but also against custom iterators
or application-specific sources.

The input interface is a low-level extension point intended for advanced use cases.
If you only need to match against strings, the built-in string overloads are usually the better and simpler choice.
They also provide a defined tolerant decoding policy: malformed units are replaced with U+FFFD.

How to Implement Your Source
============================

To implement a custom input source, derive from one of the following classes:

*   :cpp:class:`Input <erbsland::re::Input>` for UTF-8 input
*   :cpp:class:`Input16 <erbsland::re::Input16>` for UTF-16 input
*   :cpp:class:`Input32 <erbsland::re::Input32>` for UTF-32 input

The chosen base class determines which unified match family is returned: ``Match``, ``Match16`` or ``Match32``.
A custom input implements ``createMatch`` and decides whether captured content is copied or retained as a slice of its
source.

Your implementation must override the abstract methods defined by
:cpp:class:`InputBase <erbsland::re::InputBase>`. These methods form the
contract between your input source and the matching engine.

Implementation Requirements
---------------------------

The most important method is
:cpp:func:`read <erbsland::re::InputBase::read>`. It is invoked in the hot loop
of the matching engine and must therefore be implemented as efficiently as possible.

When implementing an input source, the following rules must be respected:

*   :cpp:func:`read <erbsland::re::InputBase::read>` must return the next
    character as :cpp:class:`text::Char <erbsland::text::Char>` together with
    its position. Every returned character must be a valid Unicode scalar value.

*   When the end of the input is reached,
    :cpp:func:`text::Char::endOfData <erbsland::text::Char::endOfData>` must be
    returned.

*   Repeated calls to :cpp:func:`read <erbsland::re::InputBase::read>` after the
    end of the input must continue to succeed and keep returning the end-of-data
    signal.

*   Reserved :cpp:class:`text::Char <erbsland::text::Char>` signals other than
    the end-of-data signal, surrogate code points and values above U+10FFFF must
    never be returned.

*   The returned position must advance monotonically and must uniquely identify
    the character within the input stream.

If line-break folding (CRLF handling) is enabled for the regular expression, your input source must additionally
implement:

*   :cpp:func:`peek <erbsland::re::InputBase::peek>` to look ahead without
    consuming input
*   :cpp:func:`skip <erbsland::re::InputBase::skip>` to advance by a :cpp:type:`unit::CpLength
    <erbsland::unit::CpLength>`

These methods allow the engine to treat ``CRLF`` sequences as a single logical line break while preserving correct
positional information.

The matching engine relies on this contract and does not repeat Unicode validity checks in its hot loop.
Validate or decode custom data before returning it.
Error-tolerant inputs should replace invalid source values with
:cpp:func:`text::Char::replacement <erbsland::text::Char::replacement>`.
Incorrect or incomplete implementations may lead to incorrect matches or undefined behavior.

Exception Propagation
---------------------

A custom input may choose strict decoding and throw a Core encoding error.
Exceptions thrown by ``read()``, ``peek()``, ``skip()`` or ``createMatch()`` propagate unchanged through every matching
operation.
The engine does not replace, wrap or translate these exceptions into :cpp:class:`RegExError <erbsland::re::RegExError>`.

Example Implementation
======================

The following example shows a complete implementation of a custom input source that reads characters from a
``std::vector``.
While simplified, it demonstrates all required methods and lifetime rules.
The example treats the vector as an error-tolerant source and replaces invalid UTF-32 values while reading and capturing
its content.

.. literalinclude:: files/input.hpp
    :language: cpp
    :linenos:

.. button-ref:: error
    :ref-type: doc
    :color: success
    :align: center
    :expand:
    :class: sd-fs-5 sd-font-weight-bold sd-p-2 sd-my-4

    Errors →

Interface
=========

.. doxygenstruct:: erbsland::re::CharAndPosition
    :members:
.. doxygenclass:: erbsland::re::Input
    :members:

.. doxygentypedef:: erbsland::re::InputPtr
.. doxygenclass:: erbsland::re::Input16
    :members:

.. doxygentypedef:: erbsland::re::Input16Ptr
.. doxygenclass:: erbsland::re::Input32
    :members:

.. doxygentypedef:: erbsland::re::Input32Ptr
.. doxygenclass:: erbsland::re::InputBase
    :members:

.. doxygentypedef:: erbsland::re::InputBasePtr
.. doxygentypedef:: erbsland::re::InputPosition
