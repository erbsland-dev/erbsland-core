*****************************
Options Domain API Guidelines
*****************************

Core Semantics
==============

.. code-block:: text

    option = named flag, typed value, choice, or positional argument
    option set = group of option definitions
    module = named action-specific collection of option sets

Primary Types
=============

.. code-block:: text

    Options // root definition for main option sets and modules
    OptionManager // command-line parser and display orchestrator
    OptionSet // reusable group of option definitions and parsing callbacks
    OptionModule // command module with option sets and optional main function
    Option // one named command-line definition

Definition Types
================

.. code-block:: text

    OptionEditor, OptionChoiceEditor // fluent option and choice definition editors
    OptionSetManager // shared definition-owning interface of roots, sets, and modules
    OptionChoice, OptionChoices // one accepted choice and its collection
    OptionHelp // title, description, example, epilog, and visibility
    OptionType, OptionFlag, OptionFlags // declared value kind and definition flags
    OptionHelpVisibility // inherited or explicit help visibility
    OptionParserFlag, OptionParserFlags // built-in parser behavior flags

Result Types
============

.. code-block:: text

    OptionResult // parse status, selected module, values, and error context
    OptionValues // parsed lookup map
    OptionValue // one parsed value with source definition and argument indexes
    OptionValueStorage, OptionValueType // parsed storage and classification
    OptionInteger // signed integer representation used by option values
    OptionSensitiveTextLocation, OptionSensitiveTextLocations // protected source suffixes to mask
    OptionResultStatus // success, display request, or error result

Callback and Error Types
========================

.. code-block:: text

    OptionValidateFn // validation callback for one parsed value
    PreOptionSetParsingFn, PreOptionModuleParsingFn // callbacks before definition parsing
    PostParsingFn, ModuleMainFn // callbacks after parsing or for selected-module execution
    OptionError // throwing parse or definition failure
    OptionErrorContext, OptionErrorReason // structured failure context and reason

Pattern Definitions
===================

.. code-block:: text

    Dp = OptionsPtr/OptionSetPtr/OptionModulePtr/OptionPtr // shared definition pointer
    Vp = OptionValuesPtr/OptionValuePtr // shared parsed-value pointer

Definition Construction Patterns
================================

.. code-block:: text

    T::create([primary, details]) -> Dp // create a shared root, set, module, or option definition
    o.addOption(names) -> OptionEditor // add an option and return its fluent editor
    o.editOption(name) -> OptionEditor // edit an existing option by lookup name
    o.addSet(set)/addModule(module) // add an owned option set or module
    o.optionSets()/optionModules()/options() -> T // inspect owned definitions
    o.defaultOptionSet() -> OptionSetPtr // access the lazily created direct-addition set

Definition Name Patterns
========================

.. code-block:: text

    T(names) // create an option or module and derive its identity
    o.names() -> T // inspect accepted names
    o.setNames(names)/addName(name) // replace or add accepted names
    T::is❮NameForm❯(name) -> bool // classify long, short, option, or positional names
    T::isValid❮NameForm❯(name) -> bool // validate a supported name form
    o.has❮NameForm❯(name) -> bool // test a configured name or positional identity
    o.hasConflictingOptionName(other) -> bool // test definition-name conflicts

Definition Property Patterns
============================

.. code-block:: text

    o.❮property❯()/set❮Property❯(value) // get or set a definition property
    o.set❮Property❯(value) -> OptionEditor& // fluently update an option definition
    o.setFlag/clearFlag(flag) -> OptionEditor& // add or remove one definition flag
    o.addChoice(text) -> OptionEditor& // add an accepted choice and select choice type
    o.addChoice(text/choice) -> OptionChoiceEditor // append to OptionChoices and edit the accepted choice
    o.setHelp❮Part❯(text) -> OptionChoiceEditor& // fluently update choice help metadata
    o.clearDefaultValue() -> OptionEditor& // remove a configured default
    o.isDisabled()/hasDefaultValue() -> bool // test common definition states
    o.matchingChoiceText(text) -> text::String // resolve a case-insensitive accepted choice

Parsing Patterns
================

.. code-block:: text

    T([options]) // create a manager with a new or existing root definition
    o.parse(arguments) -> OptionResult // parse, mask protected text, and return an explicit status
    o.parseOrThrow(arguments) -> OptionValuesPtr // parse and mask or throw OptionError
    T::convertCommandLineArguments(argc, argv) -> core::CommandLineArguments // convert the native process boundary
    o.options() -> OptionsPtr // access the root definition
    o.displayText()/setDisplayTextMap(map) // inspect or replace display wording

Display Patterns
================

.. code-block:: text

    o.helpDocument/versionDocument(module) -> text::TextDocument // build a neutral display document
    o.detailedHelpDocument(module, name)/moduleOverviewDocument() -> text::TextDocument // build focused help
    o.errorDocument(context) -> text::TextDocument // build a neutral error document
    o.displayHelp/displayDetailedHelp/displayVersion(module) // render a built-in display request
    o.displayModuleOverview() // render the reduced module-selection document
    o.displayError(context) // render a structured option error
    o.executablePath()/setExecutablePath(path) // retain argv zero and derive the usage name

Result Patterns
===============

.. code-block:: text

    o.status()/values()/helpName()/errorContext() -> T // inspect the explicit parse outcome
    o.moduleName()/module() -> T // inspect the selected module identity and definition
    o.sensitiveTextLocations() -> OptionSensitiveTextLocations // inspect every suffix masked after parsing
    o.value(name) -> OptionValuePtr // find a parsed value through any accepted name
    o.valueCount(name) -> unit::ArgumentCount // count parsed values for a lookup name
    o.argumentIndexes()/argumentIndex() -> T // inspect source command-line positions

Typed Value Access Patterns
===========================

.. code-block:: text

    o.getFlag([name, fallback]) -> bool // read a flag or fallback
    o.getFlagCount(name[, fallback]) -> unit::ArgumentCount // read flag occurrence count
    o.getBoolean/getInteger/getText([name, fallback]) -> T // read one typed value; sensitive text remains marked
    o.getBooleanList/getIntegerList/getTextList([name, fallback]) -> T // read all typed values
