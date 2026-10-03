************************************
Named Constant for Repeated Literals
************************************

:Rule ID: ``named_constant_for_repeated_literals``
:Severity: low

When a string key is used repeatedly in one implementation unit, give it one name.
This keeps option, configuration, and value-access keys consistent when they are changed.

Correct Solution
================

Define a ``StringLiteral`` constant with a name that describes the key, then pass that constant at each use.
Place it in the smallest scope that contains all uses, inside the relevant class or namespace.

Location of the Constants
-------------------------

*   Whenever possible, add the constants to the class that uses them.
    Add private constants to the initial private block, immediately after any friend declarations, as shown in
    Example 1.
    A type needed by a constant may precede it.
    Do not place constants after methods or among instance data members.
*   When several classes use the same keys, collect their constants in a separate header, inside a namespace with a
    name describing their shared purpose.

Document each constant with a one-line ``///`` comment before its declaration.
Describe the option, setting, or lookup it identifies rather than repeating its name or merely writing "Configuration
key".
For example: "Option key for --output-only, printing one formatted file to stdout without changing it."
Consumer code, including maintenance tools and demos, should import ``el::text::literals`` inside the owning namespace
and use ``"..."_el`` rather than explicit ``el::StringLiteral`` construction.

Naming Schema
-------------

The idea of having named constants is to provide extra information to the person reading the code.

- For an option value, use ``c<Identifier>Option``. E.g. ``file-path`` => ``cFilePathOption``
- For a configuration value use ``c<Identifier>Value``.
- For other container kinds, choose a matching suffix that tells the reader the usage of the constant.
- If no other suffix matches, use the generic ``c<Identifier>Key``.

Example 1
---------

MyComponent.hpp (when used inside our library):

.. code-block:: cpp

    namespace my_namespace {

    class MyComponent {
        /// Option key selecting the project name used in generated output.
        constexpr static auto cProjectNameKey = text::StringLiteral{"project-name"};

    public:
        // ...
    };

    }

MyComponent.hpp (when used in consumer/demo code):

.. code-block:: cpp

    namespace my_namespace {

    using namespace el::text::literals;

    class MyComponent {
        /// Option key selecting the project name used in generated output.
        constexpr static auto cProjectNameKey = "project-name"_el;

    public:
        // ...
    };

    }

MyComponent.cpp:

.. code-block:: cpp

    namespace my_namespace {

    void MyComponent::registerOptions() {
        // ...
        options.addOption(cProjectNameKey);
        // ...
    }

    void MyComponent::parseValues() {
        // ...
        const auto name = values.getText(cProjectNameKey);
        // ...
    }

    }

Example 2
---------

MyComponentConfigurationKeys.hpp (consumer code):

.. code-block:: cpp

    namespace my_namespace::key {

    using namespace el::text::literals;

    /// Configuration key selecting the project name used in generated output.
    constexpr static auto cProjectName = "project-name"_el;

    }

Mechanical Detection
====================

The scanner reports the second use of the same ``"..."_el`` literal in a ``.cpp`` file when it is a key argument to an
option, rule, or value access call.
It also skips comments, directives, and unrelated display text.
The optional ``Anti Patterns . Named Constant For Repeated Literals`` section supplies ``Key Functions`` for exact
function names and ``Key Function Prefixes`` for prefix matches.
These case-sensitive lists are empty when omitted; the shipped configuration selects the project's accessors.
Demo findings are accepted through the configured ``demos/**`` exception.
