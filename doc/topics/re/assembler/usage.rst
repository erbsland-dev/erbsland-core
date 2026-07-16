************************
How to use the Assembler
************************

There is a public API that allows you to use the assembler and disassembler in your programs.

.. note::

    Please note, that when you use the diagnostic tools, it will add additional code and data to your program that is usually removed by the compiler and linker.

.. code-block:: cpp

    #include <erbsland/re/diagnostics/Assembler.hpp>

    using el::re::Assembler;

    void main() {
        Assembler assembler;
        auto regEx = assembler.compile({
            "CHAR 'a'",
            "MATCH"
        });

        auto match = regEx->match(u8"abc");
        // ...
    }
