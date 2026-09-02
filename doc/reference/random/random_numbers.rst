..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Random Numbers; Reference
    single: Random

**************
Random Numbers
**************

:cpp:class:`Random <erbsland::random::Random>` is the abstract base class for all random generators.
See :ref:`random-overview` for generator selection and :ref:`random-working-with-numbers` for common value helpers.
``SecureRandom::isSecure()`` returns ``true``, so inherited ``buildString()``, ``buildByteBuffer()``, and
``buildByteBlock()`` select sensitive storage before generated data is written.
Non-secure generators return ordinary strings and byte storage through the same APIs.

Interface
=========

.. doxygenclass:: erbsland::random::FastRandom
    :members:
.. doxygenclass:: erbsland::random::Random
    :members:
.. doxygenclass:: erbsland::random::RandomError
    :members:
.. doxygenclass:: erbsland::random::SecureRandom
    :members:
.. doxygenclass:: erbsland::random::ThreadSafeFastRandom
    :members:
