Design Notes
============

The lexer code is deliberately procedural due to its size and the tight coupling between components. The `Lexer` class serves primarily as a container to hold the decoder instance during parsing. It also encapsulates the parsing logic for the root context of the configuration document.

The lexer is broken down into many small, focused units to enable efficient reuse of individual scanners and parsers.

Naming Scheme
-------------

The primary lexing methods follow a consistent naming pattern: `expect<Subject>` and `scan<Subject>`.

- `expect<Subject>` methods assert that a specific subject appears next in the character stream. If the expectation is not met, they throw an error. These methods return a `Token` or `TokenGenerator`.

- `scan<Subject>` methods attempt to locate the subject in the character stream. They start a transaction at the current position, and if the subject is not found, the transaction is rolled back. These methods are non-committal and return a `std::optional<Token>`.

`scan` methods may still throw if the subject is present but syntactically incorrect.

