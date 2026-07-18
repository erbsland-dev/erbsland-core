.. index::
    single: String Types

************
String Types
************

Introduction
============

:cpp:type:`String <erbsland::text::String>` is the primary string type used throughout the library, defined as
:cpp:class:`U8String <erbsland::text::U8String>`.
It stores an owning, read-only UTF-8 value with copy-on-write storage.

Use :cpp:type:`StringEditor <erbsland::text::StringEditor>` when text must be modified in place.

For a full description of the underlying type, see :doc:`string_width_variants`.

Interface
=========

.. doxygentypedef:: erbsland::text::ProcessCharacterFn
.. doxygentypedef:: erbsland::text::String
.. doxygentypedef:: erbsland::text::StringEditor
.. doxygentypedef:: erbsland::text::TransformCharacterFn
