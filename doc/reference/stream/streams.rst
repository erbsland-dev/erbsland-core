.. index::
    single: Streams

*******
Streams
*******

Interface
=========

.. doxygenclass:: erbsland::stream::ByteInputStream
    :members:
.. doxygenclass:: erbsland::stream::ByteOutputStream
    :members:
.. doxygenclass:: erbsland::stream::InputStream
    :members:
.. doxygenclass:: erbsland::stream::OutputStream
    :members:
.. doxygenfunction:: erbsland::stream::stdOut() -> TextOutputStreamPtr

.. doxygenfunction:: erbsland::stream::stdErr() -> TextOutputStreamPtr

.. doxygenfunction:: erbsland::stream::io::write(text::Char character)

.. doxygenfunction:: erbsland::stream::io::write(const text::StringView &text)

.. doxygenfunction:: erbsland::stream::io::writeLine()

.. doxygenfunction:: erbsland::stream::io::writeLine(const text::StringView &text)

.. doxygenfunction:: erbsland::stream::io::print(const tArgs &...args)

.. doxygenfunction:: erbsland::stream::io::printLine(const tArgs &...args)

.. doxygenfunction:: erbsland::stream::io::printError(const tArgs &...args)

.. doxygenfunction:: erbsland::stream::io::printErrorLine(const tArgs &...args)
.. doxygenclass:: erbsland::stream::StringBuilderStream
    :members:
.. doxygenclass:: erbsland::stream::TextInputStream
    :members:
.. doxygenclass:: erbsland::stream::TextOutputStream
    :members:
.. doxygenclass:: erbsland::stream::TextPrintContext
    :members:
