.. index::
    single: Random

******
Random
******

:cpp:class:`Random <erbsland::random::Random>` is the abstract base class for all random generators.
See :ref:`random-overview` for generator selection and :ref:`random-working-with-numbers` for common value helpers.

Interface
=========

.. doxygenclass:: erbsland::random::FastRandom
    :members:
.. doxygenclass:: erbsland::random::Random
    :members:
.. doxygenclass:: erbsland::random::SecureRandom
    :members:
.. doxygenclass:: erbsland::random::ThreadSafeFastRandom
    :members:
