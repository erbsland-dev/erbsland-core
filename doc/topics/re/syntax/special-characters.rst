.. index::
    single: Syntax; special characters
    single: Special characters
    single: Escape sequences; special characters

******************
Special Characters
******************

Special character escapes let you insert control characters and Unicode code points directly into your patterns.

They are useful when you need to match characters that are hard to type, hard to see, or ambiguous in source code (for
example line breaks or non-ASCII characters).

:esc_code:`n`
=============

Inserts a line feed character (LF, :unicode:`A` ).

Use this escape whenever your pattern needs to work with line-oriented input.

If you need Windows-style line breaks (CRLF) to behave like a single newline, enable the ``Flag::CRLF`` flag when
compiling the pattern.
With this flag enabled, the engine treats any :expression:`\\r\\n` sequence as :esc_code:`n` during matching.

:esc_code:`r`
=============

Inserts a carriage return character (CR, :unicode:`D` ).

:esc_code:`t`
=============

Inserts a horizontal tab character (TAB, :unicode:`9` ).

:esc_code:`u<hhhh>`
===================

Inserts the Unicode character with the given code point.

* ``hhhh`` must be exactly four hexadecimal digits (0–9, a–f, A–F).
* If the resulting value is outside the valid Unicode range, a parse error is raised.

This form is convenient for characters in the Basic Multilingual Plane (BMP).

Examples:

* :esc_code:`u0000` inserts U+0000 (NUL)
* :esc_code:`u0041` inserts ``A`` (:unicode:`41`)
* :esc_code:`u00E9` inserts ``é`` (:unicode:`E9`)

:esc_code:`u{<hh…>}`
====================

Inserts the Unicode character with the given code point.

* ``hh…`` is a variable-length hexadecimal number, terminated by ``}``.
* The value must be within the valid Unicode range, otherwise a parse error is raised.

This form is convenient for code points below :unicode:`100` and beyond :unicode:`FFFF`.

Examples:

* :esc_code:`u{7}` inserts BEL (:unicode:`7`)
* :esc_code:`u{1F600}` inserts ``😀`` (:unicode:`1F600`)
* :esc_code:`u{10FFFF}` inserts the highest valid Unicode code point.

Legacy and Compatibility Expressions
====================================

The following escape sequences exist for compatibility with other engines.
They are supported only as legacy syntax and should not be used for new patterns.

Prefer :esc_code:`n`, :esc_code:`r`, :esc_code:`t` and the Unicode escapes
:esc_code:`u<hhhh>` / :esc_code:`u{<hh…>}` instead.

:esc_code:`a`
-------------

Inserts the bell character (BEL, :unicode:`7` ).

:esc_code:`c<X>`
----------------

Inserts an ASCII control character.

``X`` must be an ASCII letter (A–Z or a–z).
The resulting character is computed as::

    X & 0x1F

Example:

* :esc_code:`cJ` inserts line feed (LF, :unicode:`A`)

:esc_code:`e`
-------------

Inserts the escape character (ESC, :unicode:`1B` ).

:esc_code:`f`
-------------

Inserts the form feed character (FF, :unicode:`C` ).

:esc_code:`N`
-------------

Matches any character **except** a line feed (LF, :unicode:`A` ).

This is a compatibility escape found in some engines.
For new patterns, prefer an explicit negated class such as :expression:`[^\\n]` when you want “not newline”.

:esc_code:`o{<ddd…>}`
---------------------

Inserts the character with the given octal code.

* ``ddd…`` is a variable-length octal number (digits 0–7), terminated by ``}``.
* The value must be within the valid Unicode range, otherwise a parse error is raised.

:esc_code:`x<hh>`
-----------------

Inserts the character with the given hexadecimal code.

* ``hh`` must be exactly two hexadecimal digits (0–9, a–f, A–F).
* The value must be within the valid Unicode range, otherwise a parse error is raised.

:esc_code:`x{<hh…>}`
--------------------

Inserts the character with the given hexadecimal code.

* ``hh…`` is a variable-length hexadecimal number, terminated by ``}``.
* The value must be within the valid Unicode range, otherwise a parse error is raised.

:esc_code:`U<hhhhhhhh>`
-----------------------

Inserts the Unicode character with the given code point.

* ``hhhhhhhh`` must be exactly eight hexadecimal digits (0–9, a–f, A–F).
* The value must be within the valid Unicode range, otherwise a parse error is raised.

For new patterns, prefer :esc_code:`u{<hh…>}`.
