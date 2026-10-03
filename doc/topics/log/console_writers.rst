.. index::
    single: Logging; Console Writer
    single: Log Writers; Console
    single: Console Logging; Paragraph Layout
    single: Console Logging; Styles

***************************
Writing Logs to the Console
***************************

The ``LogWriter::createForConsole()`` turns a formatted log line into a terminal paragraph.
It is the writer people usually meet first: a command starts, useful progress appears immediately, warnings stand out,
and an error is visible at the place where the person can act on it.
That makes console logging a natural fit for command-line tools, foreground services, development builds, and any
application whose operator is currently watching a terminal.

Console output has a different job from a file or a remote collector.
It should remain readable at the terminal's current width, use color only when the terminal supports it, and avoid
turning one unusually long message into a wall of text.
The console writer therefore combines two kinds of configuration: paragraph options control physical layout, while
layered styles give severity and semantic fields a visual identity.

This page starts with a normal application setup and then explains every option on
:cpp:class:`ConsoleLogWriterOptions <erbsland::log::ConsoleLogWriterOptions>`.
For the routing filter that decides which entries reach this writer, see :doc:`using_writers`.

Add Console Output to an Application
====================================

A console writer requires a :cpp:class:`Terminal <erbsland::cterm::Terminal>`.
In an :cpp:class:`Application <erbsland::core::Application>`, call
:cpp:func:`application().terminal() <erbsland::core::Application::terminal>` and pass the returned shared terminal to
the writer constructor.
The application's default logging setup already does this for information, warning, and error messages.
You create a console writer yourself when you want a different route, paragraph layout, or style palette.

The usual setup has four visible pieces.
Create :cpp:class:`ConsoleLogWriterOptions <erbsland::log::ConsoleLogWriterOptions>`, construct the writer with the
terminal and those options, add it through :cpp:func:`addWriter() <erbsland::log::LogConfiguration::addWriter>`, and
install the complete configuration in the application's manager.
The line format remains a manager-wide choice; the console writer receives the already formatted semantic parts.

This example uses a small continuation indent so a wrapped message stays visually connected to its prefix.
It relies on :cpp:class:`Application <erbsland::core::Application>` to shut the manager down and flush the terminal when
the process exits.

.. erbsland-demo::
    :source: log/LoggingTopics/ConsoleWriters.cpp
    :exec: log/logging_topics --demo ConsoleWriters
    :sha256: c14c1b5629b66b3729141f7dfafa8d1f04010db1fe003e52cee02dc22ec85693

.. code-block:: cpp

    /// A console writer renders formatted log lines as terminal paragraphs.
    ///
    /// Create its options, pass them to the writer together with the application terminal, and add the writer to the
    /// complete log configuration. The application owns and shuts down its log manager automatically.
    void consoleWriters() {
        auto paragraph = el::cterm::ParagraphOptions{};
        paragraph.setWrappedLineIndent(4);

        auto writerOptions = el::ConsoleLogWriterOptions{};
        writerOptions.setParagraphOptions(std::move(paragraph));

        auto format = el::LogLineFormat{};
        format.setPattern("{level} [{name}] {message}"_el);
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(format))
            .addWriter(el::LogWriter::createForConsole(el::application().terminal(), writerOptions));

        auto &manager = el::application().log();
        manager.setConfiguration(std::move(configuration));
        const auto log = manager.createStream("guild/weather"_el);
        log->info("Northern ridge observation opened."_el);
        log->warn("A snow squall is crossing the ridge."_el);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m␛[97;49mINF [␛[95;49mguild/weather␛[97;49m] Northern ridge observation opened.
        ␛[93;49mWRN [␛[95;49mguild/weather␛[93;49m] A snow squall is crossing the ridge.␛[0m

.. erbsland-demo-end::

When configuration comes from ELCL, construct
:cpp:class:`LogConfigurationParser <erbsland::log::LogConfigurationParser>` with the same terminal.
A parser without a terminal rejects a console writer because there would be nowhere to render it.

Console Log Writer Options
==========================

:cpp:class:`ConsoleLogWriterOptions <erbsland::log::ConsoleLogWriterOptions>` is a value object.
Construct it with its default constructor, change the settings you need, and pass it as the second argument to
``LogWriter::createForConsole()``.
The writer stores its own options value, so moving the completed object into the constructor is the clearest ownership
handoff.

The default options are deliberately useful without customization.
Paragraphs are left-aligned with no indentation and unlimited wrapping.
The base line style inherits the terminal's current appearance.
Trace, information, warning, and error lines receive bright black, bright white, bright yellow, and bright red
foregrounds respectively.
Timestamp parts are bright cyan, stream names are bright magenta, and the remaining semantic parts inherit their line
style.

From there, the options form a four-layer style model: the base line style, the selected level style, the selected
semantic-part style, and finally the part-and-level style.
Each later layer changes only the color components and attributes it explicitly specifies.
This is why a part can change its foreground while retaining a background established for the complete line.

Control Paragraph Layout
========================

:cpp:func:`setParagraphOptions() <erbsland::log::ConsoleLogWriterOptions::setParagraphOptions>` replaces the complete
:cpp:class:`ParagraphOptions <erbsland::cterm::ParagraphOptions>` value used for every log entry.
Start from a fresh paragraph value, configure it, and move it into the console options.
If you want to adjust the current writer defaults, copy the value returned by
:cpp:func:`paragraphOptions() <erbsland::log::ConsoleLogWriterOptions::paragraphOptions>` first; the accessor itself is
read-only.

The line, first-line, and wrapped-line indents solve slightly different layout problems.
:cpp:func:`setLineIndent() <erbsland::cterm::ParagraphOptions::setLineIndent>` establishes the general left indent.
:cpp:func:`setFirstLineIndent() <erbsland::cterm::ParagraphOptions::setFirstLineIndent>` can override it for the first
physical line, and :cpp:func:`setWrappedLineIndent() <erbsland::cterm::ParagraphOptions::setWrappedLineIndent>` controls
automatic continuation lines.
The special value :cpp:var:`ParagraphOptions::cUseLineIndent <erbsland::cterm::ParagraphOptions::cUseLineIndent>`, which
is the default for both overrides, reuses the general line indent.

:cpp:func:`setMaximumLineWraps() <erbsland::cterm::ParagraphOptions::setMaximumLineWraps>` bounds automatic wrapping
for one source line.
Zero, the default, means unlimited wrapping.
A positive value truncates after that many wraps and appends the mark selected by
:cpp:func:`setParagraphEllipsisMark() <erbsland::cterm::ParagraphOptions::setParagraphEllipsisMark>`.
An embedded newline begins a new source line and resets the wrap count, so a deliberately multi-line diagnostic keeps
its structure.

The following example makes every part of this relationship visible: a general indent, a distinct first-line indent, a
deeper continuation indent, one permitted wrap, and the default single-character ellipsis.

.. erbsland-demo::
    :source: log/LoggingTopics/ConsoleWriterOptions.cpp
    :show: f:consoleWriterParagraphOptions
    :exec: log/logging_topics --demo ConsoleWriterParagraphOptions
    :sha256: 28190afe24ae1a7f263fe242c2450fc1541cf1ef2513f27f3f41b1efcdc24963

.. code-block:: cpp

    void consoleWriterParagraphOptions() {
        auto paragraph = el::cterm::ParagraphOptions{};
        paragraph.setLineIndent(2);
        paragraph.setFirstLineIndent(4);
        paragraph.setWrappedLineIndent(8);
        paragraph.setMaximumLineWraps(1);
        paragraph.setParagraphEllipsisMark(el::cterm::BlockStringEditor{"…"_el});

        auto writerOptions = el::ConsoleLogWriterOptions{};
        writerOptions.setParagraphOptions(std::move(paragraph));
        auto format = el::LogLineFormat{};
        format.setPattern("{message}"_el);
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(format))
            .addWriter(el::LogWriter::createForConsole(el::application().terminal(), writerOptions));

        auto &manager = el::application().log();
        manager.setConfiguration(std::move(configuration));
        manager.rootStream()->info(
            "The northern survey report contains a long sequence of observations that should remain compact on an "_el,
            "operator's terminal even when the available line width is limited and several route notes follow."_el);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m    ␛[97;49mThe northern survey report contains a long sequence of observations that should remain
        ␛[39;49m        ␛[97;49mcompact on an operator's terminal even when the available line width is limited␛[39;49m…␛[0m

.. erbsland-demo-end::

The entire paragraph object is passed through, not merely the four fields supported by the logging ELCL schema.
This means C++ configuration may also select alignment, margins, paragraph spacing, wrap start and end marks, word
separators, a word-break mark, tab stops and overflow behavior, background extension, and the fallback used when a
paragraph cannot be rendered.
These are general terminal-layout features rather than logging-specific rules; :doc:`../cterm/paragraph-options`
explains each one with visual examples.

Set the Base Style for Every Line
=================================

:cpp:func:`setBaseLineStyle() <erbsland::log::ConsoleLogWriterOptions::setBaseLineStyle>` establishes the first style
layer for every line.
Its default :cpp:class:`BlockStyle <erbsland::cterm::BlockStyle>` inherits foreground, background, and character
attributes from the terminal.
Set it when all log output should share a background, a common attribute, or a foreground that more specific layers may
selectively retain.

A :cpp:class:`BlockStyle <erbsland::cterm::BlockStyle>` can specify a foreground, a background, character attributes, or
any combination of them.
An inherited component leaves the layer below unchanged; a default component explicitly restores the terminal default.
This distinction matters when styles are combined.
For example, the built-in information-level overlay changes the foreground to bright white but leaves the blue base
background in the following demo untouched.

.. erbsland-demo::
    :source: log/LoggingTopics/ConsoleWriterOptions.cpp
    :show: f:consoleWriterBaseStyle
    :exec: log/logging_topics --demo ConsoleWriterBaseStyle
    :sha256: 288acce413cb15cee56f6d3248c193b9afe951f6c678c9a73be672052fc83e98

.. code-block:: cpp

    void consoleWriterBaseStyle() {
        auto writerOptions = el::ConsoleLogWriterOptions{};
        writerOptions.setBaseLineStyle(el::cterm::BlockStyle{el::cterm::fg::BrightWhite, el::cterm::bg::Blue});
        auto format = el::LogLineFormat{};
        format.setPattern("{level} {message}"_el);
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(format))
            .addWriter(el::LogWriter::createForConsole(el::application().terminal(), writerOptions));

        auto &manager = el::application().log();
        manager.setConfiguration(std::move(configuration));
        manager.rootStream()->info("The base style supplies the blue background."_el);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m␛[97;44mINF The base style supplies the blue background.␛[0m

.. erbsland-demo-end::

For the complete color and attribute model, including inherited and explicitly reset components, see
:doc:`../cterm/color`.

Style Complete Lines by Severity
================================

:cpp:func:`setLineStyle() <erbsland::log::ConsoleLogWriterOptions::setLineStyle>` replaces the level overlay for one
:cpp:class:`LogLevel <erbsland::log::LogLevel>`.
Call it once for each level whose complete-line appearance should differ from the defaults.
The style is applied after the base line style and before any semantic-part styles.

Severity styling should support the text rather than replace it.
Color may be unavailable, redirected output is plain, and readers may have different color perception.
Keep the textual level placeholder in the line pattern and use styling as a second signal.
A quiet information color, a strong warning, and a distinct error are usually more useful than giving every level an
equally intense treatment.

The example replaces three level overlays.
Only the specified foreground and attribute are changed; any inherited background still comes from the base layer.

.. erbsland-demo::
    :source: log/LoggingTopics/ConsoleWriterOptions.cpp
    :show: f:consoleWriterLevelStyles
    :exec: log/logging_topics --demo ConsoleWriterLevelStyles
    :sha256: 26e15fd80cd06b4ecacc132b4868622dd51560cb2ceeb2f3d4a7c0011d14bcc5

.. code-block:: cpp

    void consoleWriterLevelStyles() {
        auto writerOptions = el::ConsoleLogWriterOptions{};
        writerOptions.setLineStyle(el::LogLevel::Information, el::cterm::BlockStyle{el::cterm::fg::BrightGreen})
            .setLineStyle(
                el::LogLevel::Warning, el::cterm::BlockStyle{el::cterm::fg::BrightYellow, el::cterm::BlockAttributes::Bold})
            .setLineStyle(
                el::LogLevel::Error,
                el::cterm::BlockStyle{el::cterm::fg::BrightRed, el::cterm::BlockAttributes::Underline});
        auto format = el::LogLineFormat{};
        format.setPattern("{level} {message}"_el);
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(format))
            .addWriter(el::LogWriter::createForConsole(el::application().terminal(), writerOptions));

        auto &manager = el::application().log();
        manager.setConfiguration(std::move(configuration));
        const auto log = manager.rootStream();
        log->info("The expedition registry opened."_el);
        log->warn("The western trail report is overdue."_el);
        log->error("The emergency beacon did not answer."_el);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m␛[92;49mINF The expedition registry opened.
        ␛[93;49m␛[1mWRN␛[22m ␛[1mThe␛[22m ␛[1mwestern␛[22m ␛[1mtrail␛[22m ␛[1mreport␛[22m ␛[1mis␛[22m ␛[1moverdue.
        ␛[91;49m␛[4mERR␛[24m ␛[4mThe␛[24m ␛[4memergency␛[24m ␛[4mbeacon␛[24m ␛[4mdid␛[24m ␛[4mnot␛[24m ␛[4manswer.␛[0m

.. erbsland-demo-end::

Semantic Field Styles across Severity Levels
============================================

The two-argument :cpp:func:`setPartStyle() <erbsland::log::ConsoleLogWriterOptions::setPartStyle>` overload assigns a
base style to one :cpp:class:`LogLinePart <erbsland::log::LogLinePart>` across every severity.
Available parts are literal pattern text, timestamp, level, stream name, and message.
The line formatter preserves these identities while assembling the displayed text, which lets the console writer color
the stream name without searching the final string.

Part styles are most helpful for stable fields that readers scan repeatedly.
A consistent stream-name color makes interleaved module output easier to follow, while a subtle timestamp avoids
competing with the message.
Literal punctuation can normally inherit the line style so the display does not become visually fragmented.

This example changes three semantic parts.
The italic message inherits its foreground from the information-level line, while level and name supply their own
foregrounds.

.. erbsland-demo::
    :source: log/LoggingTopics/ConsoleWriterOptions.cpp
    :show: f:consoleWriterPartStyles
    :exec: log/logging_topics --demo ConsoleWriterPartStyles
    :sha256: 804c0356bbfda30e7eb6b6b481b920d4bc72226c4d18eaa7ec91340bb03494ad

.. code-block:: cpp

    void consoleWriterPartStyles() {
        auto writerOptions = el::ConsoleLogWriterOptions{};
        writerOptions.setPartStyle(el::LogLinePart::Level, el::cterm::BlockStyle{el::cterm::fg::BrightGreen})
            .setPartStyle(el::LogLinePart::Name, el::cterm::BlockStyle{el::cterm::fg::BrightCyan})
            .setPartStyle(el::LogLinePart::Message, el::cterm::BlockStyle{el::cterm::BlockAttributes::Italic});
        auto format = el::LogLineFormat{};
        format.setPattern("{level} [{name}] {message}"_el);
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(format))
            .addWriter(el::LogWriter::createForConsole(el::application().terminal(), writerOptions));

        auto &manager = el::application().log();
        manager.setConfiguration(std::move(configuration));
        manager.createStream("guild/archive"_el)->info("The route ledger was indexed."_el);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m␛[92;49mINF␛[97;49m [␛[96;49mguild/archive␛[97;49m] ␛[3mThe␛[23m ␛[3mroute␛[23m ␛[3mledger␛[23m ␛[3mwas␛[23m ␛[3mindexed.␛[0m

.. erbsland-demo-end::

Severity-Specific Field Style Overrides
=======================================

The three-argument :cpp:func:`setPartStyle() <erbsland::log::ConsoleLogWriterOptions::setPartStyle>` overload adds the
final and most specific style layer.
It takes a semantic part, a log level, and the style used only for that combination.
No part-and-level overrides are configured by default.

This layer is useful when a whole line should remain calm but one field deserves stronger emphasis.
For example, an error line can keep a neutral prefix while the actual failure message becomes bright red and bold.
Because this is an overlay, the name style and literal punctuation remain independent.

.. erbsland-demo::
    :source: log/LoggingTopics/ConsoleWriterOptions.cpp
    :show: f:consoleWriterLevelPartStyles
    :exec: log/logging_topics --demo ConsoleWriterLevelPartStyles
    :sha256: 498bfe748420e93bd61dcd0673936b469f8caea5936a5099e6ebfc9fab572e94

.. code-block:: cpp

    void consoleWriterLevelPartStyles() {
        auto writerOptions = el::ConsoleLogWriterOptions{};
        writerOptions.setLineStyle(el::LogLevel::Error, el::cterm::BlockStyle{el::cterm::fg::BrightWhite})
            .setPartStyle(el::LogLinePart::Name, el::cterm::BlockStyle{el::cterm::fg::BrightCyan})
            .setPartStyle(
                el::LogLinePart::Message,
                el::LogLevel::Error,
                el::cterm::BlockStyle{el::cterm::fg::BrightRed, el::cterm::BlockAttributes::Bold});
        auto format = el::LogLineFormat{};
        format.setPattern("{level} [{name}] {message}"_el);
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(format))
            .addWriter(el::LogWriter::createForConsole(el::application().terminal(), writerOptions));

        auto &manager = el::application().log();
        manager.setConfiguration(std::move(configuration));
        manager.createStream("guild/dispatch"_el)->error("No guide is available for the eastern pass."_el);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m␛[97;49mERR [␛[96;49mguild/dispatch␛[97;49m] ␛[91;49m␛[1mNo␛[22m ␛[1mguide␛[22m ␛[1mis␛[22m ␛[1mavailable␛[22m ␛[1mfor␛[22m ␛[1mthe␛[22m ␛[1meastern␛[22m ␛[1mpass.␛[0m

.. erbsland-demo-end::

Remain Readable Without Terminal Styling
========================================

The writer asks the terminal how output should be rendered.
On an interactive terminal the log entries are rendered with colors.
When output is redirected or the terminal selects plain block-text output, the same entries are written without style
sequences.

This fallback is why layout and text remain the primary design tools.
Choose a useful line pattern, keep levels visible, and write messages that make sense without color.
Then use console options to improve scanning and emphasis for readers who do have an interactive terminal.
