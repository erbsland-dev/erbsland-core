*****************************
Options Domain API Guidelines
*****************************

These guidelines extend the Common, Text, Unit and Value, and Error API Guidelines for public APIs that define, parse,
and render command line options.

The purpose of this document is to define a base naming vocabulary for options APIs.
It is intentionally plain, technical and list based to get a quick overview of method names and their usage patterns.
If you introduce new vocabulary, update this page to provide a good reference for future extensions.

Core Semantics
==============

Command-Line Vocabulary
-----------------------

.. code-block:: text

    option // named command-line value, flag, choice, or positional argument
    regular option // option with at least one dashed name
    positional argument // option without dashed names
    module // first command-line token that selects an action-specific option set
    option set // group of option definitions with its own parsing callbacks
    values // parsed lookup map from accepted names to shared parsed values
    document // neutral text::TextDocument for help, version and error display
    executable path // unprocessed argv[0] text captured for rendered usage
    executable name // file name extracted from executable path, without a .exe suffix

Name Forms
----------

.. code-block:: text

    -a // short option, case-sensitive
    -abc // grouped short flags
    --long // long option, case-insensitive
    --long=value // value attached with equals, allowing values starting with dash
    positional // dashless option name or parsed positional value
    module-name // module selector, same text rules as long names
    -h, --help, --version // built-in display requests, unless disabled with parser flags

Primary Types
=============

.. code-block:: text

    Options // root configuration object for main option sets and modules
    OptionManager // parser and display orchestrator
    OptionSet // option group with parsing callbacks
    OptionModule // command-line module with option sets and optional main function
    Option // single command-line option definition
    OptionEditor // fluent editor for option definitions
    OptionResult // status, values and error context from parsing
    OptionValues // parsed value lookup map
    OptionValue // one parsed value with source option and argument indexes

Definition Types
================

.. code-block:: text

    OptionChoice // single accepted choice for a choice option
    OptionChoices // collection of accepted choices
    OptionHelp // visibility, title, description and epilog text
    OptionErrorContext // structured error reason and source location
    OptionSetManager // common interface for objects that own option definitions

Value and Callback Types
========================

.. code-block:: text

    OptionInteger // integer type used by the options system
    OptionValueStorage // variant storage for parsed or default values
    OptionValueNameHash // hash helper for value lookup names
    OptionValidateFn // callback that validates one parsed option value
    PreOptionSetParsingFn // callback before a set is parsed
    PreOptionModuleParsingFn // callback before a module is parsed
    PostParsingFn // callback after successful parsing and validation
    ModuleMainFn // main function for a selected module

Enumerations and Flags
======================

.. code-block:: text

    OptionType // declared option value type: Flag, Integer, Text, Choice
    OptionFlag, OptionFlags // Disabled, Required and Greedy definition flags
    OptionParserFlag, OptionParserFlags // DisableHelp and DisableVersion parser flags
    OptionHelpVisibility // Inherit, Hidden, Normal, Overview and Usage help levels
    OptionValueType // concrete parsed storage type
    OptionResultStatus // Success, DisplayVersion, DisplayHelp or Error
    OptionErrorReason // structured reason for option errors

Shared Pointer Types
====================

.. code-block:: text

    OptionsPtr, OptionManagerPtr
    OptionSetPtr, OptionSetWeakPtr
    OptionModulePtr
    OptionPtr, OptionWeakPtr
    OptionChoicePtr, OptionChoicesPtr
    OptionValuePtr, OptionValuesPtr

Definition Construction Patterns
================================

.. code-block:: text

    T::create() -> TPtr // create an empty shared definition/value object
    Option::create(names) -> OptionPtr // create shared option with names
    OptionChoice::create(text[, help]) -> OptionChoicePtr // create shared choice
    OptionChoices::create([choices]) -> OptionChoicesPtr // create shared choice collection
    OptionModule::create([name]) -> OptionModulePtr // create shared module
    OptionValue::create(storage[, indexes]) -> OptionValuePtr // create shared parsed value
    OptionValue::create(option, storage[, indexes]) -> OptionValuePtr // create value with source option

Definition Ownership Patterns
=============================

.. code-block:: text

    o.addOption(name-or-names) -> OptionEditor // add an option and return fluent editor
    o.editOption(name) -> OptionEditor // edit an existing option by lookup name
    o.addSet(optionSet) -> void // add a set to root options or module
    o.addModule(optionModule) -> void // add a module to root options
    o.optionSets()/optionModules() -> const vector<Ptr>& // inspect owned groups
    o.executablePath()/setExecutablePath(path) // get or set captured argv[0] text
    o.executableName() // inspect the extracted executable name used in usage text
    o.options() -> const vector<OptionPtr>& // inspect options in a set
    o.defaultOptionSet() -> OptionSetPtr // internal lazy default group for direct additions

Option Name Patterns
====================

.. code-block:: text

    Option(names) // create option and derive implicit type from names
    o.names() -> const vector<String>& // configured lookup names
    o.setNames(names) -> void // replace names and update implicit type
    o.addName(name) -> void // add one lookup name
    T::isLongName/isShortName(name) -> bool // classify dashed name shape
    T::isOptionName/isPositionalName(name) -> bool // classify option or dashless name
    T::isValidLongName/isValidShortName(name) -> bool // validate dashed names
    T::isValidOptionName/isValidPositionalName(name) -> bool // validate accepted lookup names
    OptionModule::isValidName(name) -> bool // validate module name
    o.hasLongName/hasShortName(name) -> bool // test configured option name
    o.hasOptionName()/isRegularOption() -> bool // test for dashed regular option names
    o.hasPositionalName([name]) -> bool // test dashless lookup or positional name
    o.isPositionalArgument() -> bool // test option without dashed names
    o.hasValidOptionNames() -> bool // test all configured names
    o.hasConflictingOptionName(other) -> bool // test name conflicts
    o.hasName(name) -> bool // test module name match

Option Definition Patterns
==========================

.. code-block:: text

    o.help()/setHelp(help) // get or set complete help data
    o.setHelpTitle(text) // set title for root, set, module or option help
    o.setHelpDescription(text) // set description for root, set, module or option help
    o.setHelpEpilog(text) // set trailing help text for roots, modules and custom renderers
    o.setHelpVisibility(visibility) // set help visibility for roots, sets, modules or options
    o.valueName()/setValueName(name) // get or set the bare value placeholder name shown in help
    o.type()/setType(type) // get or set expected value type
    o.flags()/setFlags(flags) // get or set definition flags
    o.isDisabled() -> bool // test disabled flag
    o.choices()/setChoices(choices) // get or set accepted choices
    o.matchingChoiceText(text) -> optional<String> // case-insensitive choice match
    o.maximum()/setMaximum(count) // get or set maximum value count
    o.hasDefaultValue() -> bool // test if default storage exists
    o.defaultValue()/setDefaultValue(value) // get or set default storage
    o.validateFn()/setValidateFn(fn) // get or set value validation callback
    o.preParsingFn()/setPreParsingFn(fn) // get or set pre-parse callback
    o.postParsingFn()/setPostParsingFn(fn) // get or set post-parse callback
    o.mainFn()/setMainFn(fn) // get or set selected-module main callback

Editor Patterns
===============

.. code-block:: text

    o.isValid() -> bool // test if editor references an option
    o.option() -> const OptionPtr& // access edited option
    o.setHelp(description-or-help) -> OptionEditor& // set complete option help
    o.setHelpTitle(text) -> OptionEditor& // set option help title
    o.setHelpDescription(text) -> OptionEditor& // set option help description
    o.setHelpEpilog(text) -> OptionEditor& // set option-level epilog for custom renderers
    o.setHelpVisibility(visibility) -> OptionEditor& // set option help visibility
    o.setValueName(name) -> OptionEditor& // set the bare value placeholder name shown in help
    o.setType(type) -> OptionEditor& // set expected option type
    o.setFlags(flags) -> OptionEditor& // replace option flags
    o.setFlag(flag) -> OptionEditor& // add one option flag
    o.clearFlag(flag) -> OptionEditor& // remove one option flag
    o.setChoices(choices) -> OptionEditor& // set choices and promote to Choice type
    o.addChoice(text) -> OptionEditor& // add choice and promote to Choice type
    o.setMaximum(count) -> OptionEditor& // set maximum value count
    o.setDefaultValue(value) -> OptionEditor& // set default value
    o.clearDefaultValue() -> OptionEditor& // remove default value
    o.setValidateFn(fn) -> OptionEditor& // set validation callback

Parsing and Display Patterns
============================

.. code-block:: text

    OptionManager() // create manager with empty options root
    OptionManager(options) // create manager for existing root options
    o.options() -> const OptionsPtr& // access root options
    o.displayText()/setDisplayTextMap(map) // get or set help, version and error wording
    o.parse(args-or-argc-argv) -> OptionResult // parse and return status
    o.parseOrThrow(args-or-argc-argv) -> OptionValuesPtr // parse or throw options::OptionError
    o.helpDocument(moduleName) -> text::TextDocument // build help output document
    o.versionDocument(moduleName) -> text::TextDocument // build version output document
    o.errorDocument(errorContext) -> text::TextDocument // build option-error output document
    o.displayHelp(moduleName) -> void // render help
    o.displayVersion(moduleName) -> void // render version
    o.displayError(errorContext) -> void // render option error
    T::convertCommandLineArguments(argc, argv) -> core::CommandLineArguments // convert process arguments

Result and Value Patterns
=========================

.. code-block:: text

    o.values()/setValues(values) // get or set parsed values on a result
    o.status()/setStatus(status) // get or set parse result status
    o.errorContext()/setErrorContext(context) // get or set parse error context
    o.moduleName()/setModuleName(name) // selected module lookup name
    o.module()/setModule(module) // selected module object
    o.setValue(name-or-names, value) -> void // add parsed value lookup
    o.valueCount(name) -> unit::ArgumentCount // number of stored values for a lookup name
    o.value(name) -> OptionValuePtr // parsed value for a lookup name
    o.values() -> const unordered_map& // inspect all parsed value mappings
    o.storage()/setStorage(storage) // get or set concrete value storage
    o.argumentIndexes()/setArgumentIndexes(indexes) // get or set source argument indexes
    o.argumentIndex() -> unit::ArgumentIndex // first source argument index
    o.flagCount()/valueCount() -> unit::ArgumentCount // source occurrence or stored value count
    o.type() -> OptionValueType // concrete parsed storage type

Typed Value Access Patterns
===========================

.. code-block:: text

    o.getFlag([name, ][default]) -> bool // read flag value or default
    o.getFlagCount(name[, default]) -> unit::ArgumentCount // read flag occurrence count or default
    o.getInteger([name, ][default]) -> OptionInteger // read integer value or default
    o.getText([name, ][default]) -> text::String // read text value or default
    o.getTextList([name, ][default]) -> vector<text::String> // read text list or default
    o.getIntegerList([name, ][default]) -> vector<OptionInteger> // read integer list or default

Help and Error Context Patterns
===============================

.. code-block:: text

    OptionHelp(description) // create help from description text
    o.visibility()/setVisibility(value) // get or set help visibility
    o.title()/setTitle(text) // get or set help title
    o.description()/setDescription(text) // get or set help description
    o.epilog()/setEpilog(text) // get or set help epilog
    o.text()/setText(text) // get or set choice text
    o.addChoice(choice-or-text) -> OptionChoices& // add accepted choice
    o.choiceCount() -> unit::ArgumentCount // number of accepted choices
    o.title()/setTitle(text) -> OptionErrorContext& // short diagnostic title
    o.description()/setDescription(text) -> OptionErrorContext& // detailed diagnostic explanation
    o.reason()/setReason(reason) -> OptionErrorContext& // structured error reason
    o.argumentIndex()/setArgumentIndex(index) -> OptionErrorContext& // source argument index
    o.arguments()/setArguments(arguments) -> OptionErrorContext& // captured command-line source
    o.options()/setOptions(options) -> OptionErrorContext& // options root retained for diagnostic rendering
    o.module()/setModule(module) -> OptionErrorContext& // selected module
    o.option()/setOption(option) -> OptionErrorContext& // source option
    o.optionSet()/setOptionSet(optionSet) -> OptionErrorContext& // source option set
    o.displayText()/setDisplayText(text) -> OptionErrorContext& // captured immutable display-text map
