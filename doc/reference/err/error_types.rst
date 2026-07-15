.. index::
    single: Error Types and Error Handling

******************************
Error Types and Error Handling
******************************

Introduction
============

Format Error
------------

:cpp:class:`FormatError <erbsland::text::FormatError>` reports invalid format patterns, mismatched format arguments,
unsupported format options, and format output limit failures.

Parse Error
-----------

:cpp:class:`ParseError <erbsland::err::ParseError>` reports invalid text syntax.
Integer parsing uses it for invalid digits, invalid or disallowed base prefixes, invalid separators, missing digits, and
strict trailing characters.
:cpp:class:`ParseNumberError <erbsland::text::ParseNumberError>` extends parse errors with the reader status that caused
numeric parsing to fail.

Interface
=========

.. doxygenclass:: erbsland::err::Exception
    :members:
.. doxygenclass:: erbsland::err::LogicError
    :members:
.. doxygenclass:: erbsland::err::OutOfRangeError
    :members:
.. doxygenclass:: erbsland::err::OverflowError
    :members:
.. doxygenclass:: erbsland::err::ParameterError
    :members:
.. doxygenclass:: erbsland::err::ParseError
    :members:
.. doxygenclass:: erbsland::err::RuntimeError
    :members:
