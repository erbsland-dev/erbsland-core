.. index::
    single: String Types

************
String Types
************

Introduction
============

:cpp:type:`String <erbsland::text::String>` is the common string type used throughout the library, defined as
:cpp:class:`U8String <erbsland::text::U8String>`.
It stores UTF-8 encoded text with copy-on-write storage.

For a full description of the underlying type, see :doc:`string_width_variants`.

Interface
=========

.. doxygentypedef:: erbsland::text::ProcessCharacterFn
.. doxygentypedef:: erbsland::text::String
.. doxygentypedef:: erbsland::text::StringView
.. doxygentypedef:: erbsland::text::TransformCharacterFn
