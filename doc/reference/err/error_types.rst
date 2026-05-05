.. index::
    single: Error Types and Error Handling

******************************
Error Types and Error Handling
******************************

Introduction
============

Format Error
------------

:cpp:class:`FormatError <erbsland::err::FormatError>` reports invalid format patterns, mismatched format arguments,
unsupported format options, and format output limit failures.

Parse Error
-----------

:cpp:class:`ParseError <erbsland::err::ParseError>` reports invalid text syntax.
Integer parsing uses it for invalid digits, invalid or disallowed base prefixes, invalid separators, missing digits, and
strict trailing characters.
:cpp:class:`ParseNumberError <erbsland::err::ParseNumberError>` extends parse errors with the reader status that caused
numeric parsing to fail.

Interface
=========

.. doxygenclass:: erbsland::err::ApplicationError
    :members:
.. doxygenclass:: erbsland::err::EncodingError
    :members:
.. doxygenclass:: erbsland::err::Exception
    :members:
.. doxygenclass:: erbsland::err::FormatError
    :members:
.. doxygenclass:: erbsland::err::OptionError
    :members:
.. doxygenclass:: erbsland::err::OutOfRangeError
    :members:
.. doxygenclass:: erbsland::err::OverflowError
    :members:
.. doxygenclass:: erbsland::err::ParseError
    :members:
.. doxygenclass:: erbsland::err::ParseNumberError
    :members:
.. doxygenclass:: erbsland::err::RandomError
    :members:
.. doxygenclass:: erbsland::err::StreamError
    :members:
.. doxygenfunction:: erbsland::err::throwEncodingError(std::string_view reason)

.. doxygenfunction:: erbsland::err::throwU8EncodingError(std::string_view reason, std::size_t index)

.. doxygenfunction:: erbsland::err::throwU16EncodingError(std::string_view reason, std::size_t index)

.. doxygenfunction:: erbsland::err::throwU32EncodingError(std::string_view reason, std::size_t index)

.. doxygenfunction:: erbsland::err::throwFormatError(std::string_view reason)

.. doxygenfunction:: erbsland::err::throwFormatError(text::StringView reason)

.. doxygenfunction:: erbsland::err::throwOutOfRange(std::string_view reason)

.. doxygenfunction:: erbsland::err::throwOutOfRange(text::StringView reason)

.. doxygenfunction:: erbsland::err::throwOverflow(std::string_view reason)

.. doxygenfunction:: erbsland::err::throwOverflow(text::StringView reason)

.. doxygenfunction:: erbsland::err::throwParseError(std::string_view reason)

.. doxygenfunction:: erbsland::err::throwParseError(text::StringView reason)
.. doxygenclass:: erbsland::err::U16EncodingError
    :members:
.. doxygenclass:: erbsland::err::U32EncodingError
    :members:
.. doxygenclass:: erbsland::err::U8EncodingError
    :members:
