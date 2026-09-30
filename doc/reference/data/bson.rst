..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: BSON Values

***********
BSON Values
***********

Introduction
============

:cpp:class:`BsonValue <erbsland::data::bson::BsonValue>` represents BSON values and reads or writes complete document
frames.
For usage, type mapping, and input limits, see :doc:`/topics/data/reading_and_writing_bson`.

Interface
=========

.. doxygenstruct:: erbsland::data::bson::BsonBinary
    :members:
.. doxygentypedef:: erbsland::data::bson::BsonArray

.. doxygentypedef:: erbsland::data::bson::BsonDocument
.. doxygenclass:: erbsland::data::bson::BsonFormatOptions
    :members:
.. doxygenclass:: erbsland::data::bson::BsonOpaqueValue
    :members:
.. doxygenclass:: erbsland::data::bson::BsonParseOptions
    :members:
.. doxygenenum:: erbsland::data::bson::BsonType
.. doxygenclass:: erbsland::data::bson::BsonValue
    :members:
