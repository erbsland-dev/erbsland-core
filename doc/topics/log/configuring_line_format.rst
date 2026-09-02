.. index::
    single: Logging; Line Format
    single: Log Lines; Patterns
    single: Log Lines; Stream Names
    single: Log Lines; Truncation

********************
Formatting Log Lines
********************

A useful log line gives its reader enough context to understand an event without making the event itself disappear in
decoration.
Interactive tools may need little more than the message, while a long-running service usually benefits from a timestamp,
severity, and stream name on every entry.
This page shows how to describe both styles with :cpp:class:`LogLineFormat <erbsland::log::LogLineFormat>`, and how to
keep long names and messages readable without confusing presentation limits with the manager's memory limits.

Place the Line Format in the Logging Configuration
==================================================

A line format is a small value object that belongs to a complete
:cpp:class:`LogConfiguration <erbsland::log::LogConfiguration>`.
Construct it locally, change the settings that matter to your application, and pass it to
:cpp:func:`LogConfiguration::setLineFormat <erbsland::log::LogConfiguration::setLineFormat>`.
When the manager receives the finished configuration, every writer route in that snapshot uses the same format.
This gives console and file output a common vocabulary even when filters send them different entries.

The following demo uses a standalone manager so the complete sequence is visible.
The console writer is not part of line formatting; it is added because a manager needs a destination before the result
can be seen.
An :cpp:class:`Application <erbsland::core::Application>` follows the same configuration sequence with its
application-owned manager.

.. erbsland-demo::
    :source: log/LoggingTopics/LineFormats.cpp
    :exec: log/logging_topics --demo LineFormats
    :source-sha256: b7d7cf78216eee5664a6d17c0403811bf7fbbac6a4bc2a2fed10bd4fe0a83cf9

.. code-block:: cpp

    /// Install one line format as part of a complete logging configuration.
    ///
    /// Create `LogLineFormat` as a value, customize it, and move it into `LogConfiguration`. The manager takes another
    /// complete configuration value and applies that same line format to every writer route in the snapshot.
    void lineFormats() {
        auto lineFormat = el::LogLineFormat{};
        lineFormat.setPattern("{level} [{name}] {message}"_el);

        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(lineFormat))
            .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        const auto log = manager->createStream("guild/routes"_el);
        log->info("The route catalog is ready."_el);
        manager->shutdown();
    }

.. erbsland-ansi::
    :escape-char: ␛

    ␛[97mINF [␛[95mguild/routes␛[97m] The route catalog is ready.␛[0m

.. erbsland-demo-end::

Arrange Fields With a Pattern
=============================

The pattern determines which pieces of an entry appear and how they are arranged.
A new :cpp:class:`LogLineFormat <erbsland::log::LogLineFormat>` starts with ``{time} {level} - {message}``.
Call :cpp:func:`LogLineFormat::setPattern <erbsland::log::LogLineFormat::setPattern>` when another layout better suits
your readers.
The pattern is not a general-purpose formatting language; it deliberately has four semantic placeholders:

.. list-table:: Available line-pattern placeholders
    :header-rows: 1

    * - Placeholder
      - Inserted value
    * - ``{time}``
      - The entry timestamp, rendered in the selected time zone.
    * - ``{level}``
      - The entry severity, rendered in the selected level format.
    * - ``{name}``
      - The hierarchical stream path, rendered in the selected name format. It is empty for the root stream.
    * - ``{message}``
      - The prepared message text, including any rendering-time truncation.

Literal text may surround the placeholders, and a placeholder may be omitted or repeated.
Write ``{{`` or ``}}`` when the result needs a literal opening or closing brace.
The setter validates the pattern immediately; an empty pattern, an unknown placeholder, or an unmatched brace raises
:cpp:class:`ParameterError <erbsland::err::ParameterError>` while the configuration is being assembled.

The demo compares three patterns: message-only output for a compact command, level and stream context for a larger
application, and a labelled format containing literal braces.

.. erbsland-demo::
    :source: log/LoggingTopics/LinePatterns.cpp
    :exec: log/logging_topics --demo LinePatterns
    :source-sha256: 720162900d04786cfcb30f1d09729d09e85b9b0ae119a4b2e845384cdcf261b6

.. code-block:: cpp

    /// Arrange semantic log fields with a validated placeholder pattern.
    ///
    /// `setPattern()` accepts time, level, name, and message placeholders. A compact command can show only the message,
    /// while larger applications often add level and name. Doubled braces produce literal braces in the output.
    void linePatterns() {
        const auto showPattern = [](const el::String &label, el::String pattern) -> void {
            el::io::printLine(label, ":"_el);
            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern(std::move(pattern));
            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(lineFormat))
                .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

            const auto manager = el::LogManager::create();
            manager->setConfiguration(std::move(configuration));
            manager->createStream("guild/routes"_el)->info("Selected the ridge route."_el);
            manager->shutdown();
        };

        showPattern("Message only"_el, "{message}"_el);
        showPattern("Level and name"_el, "{level} [{name}] {message}"_el);
        showPattern("Literal braces"_el, "{{guild}} {level}: {message}"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Message only:
    ␛[97mSelected the ridge route.
    ␛[39mLevel and name:
    ␛[97mINF [␛[95mguild/routes␛[97m] Selected the ridge route.
    ␛[39mLiteral braces:
    ␛[97m{guild} INF: Selected the ridge route.␛[0m

.. erbsland-demo-end::

Choose UTC or Local Timestamps
==============================

Log entries retain their timestamp in UTC, which makes entries from different machines and time zones straightforward to
compare.
Presentation is a separate choice.
:cpp:func:`LogLineFormat::setTimestampZone <erbsland::log::LogLineFormat::setTimestampZone>` accepts either
``LogTimestampZone::Utc`` or ``LogTimestampZone::Local`` and affects only text inserted for ``{time}``.

UTC is the default and ends its ISO timestamp with ``Z``.
Local rendering converts the same kind of retained instant through the process-local time zone and includes the
applicable numeric offset.
Local output is friendly in tools operated from one location; UTC is generally easier to correlate across services.
On a machine configured for UTC, both choices naturally display the same clock time.

.. erbsland-demo::
    :source: log/LoggingTopics/LineTimestampZones.cpp
    :exec: log/logging_topics --demo LineTimestampZones
    :source-sha256: 497baf08b2bfdbb93cab66efc877fffe15a626a5d42a9b1cbbcec972758d9f41

.. code-block:: cpp

    /// Render retained UTC timestamps either in UTC or in the process-local time zone.
    ///
    /// `setTimestampZone()` changes presentation only; the entry keeps its original UTC instant. ISO output includes `Z`
    /// for UTC or the applicable numeric offset for local time.
    void lineTimestampZones() {
        const auto showZone = [](const el::String &label, const el::LogTimestampZone zone) -> void {
            el::io::printLine(label, ":"_el);
            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern("{time} - {message}"_el).setTimestampZone(zone);
            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(lineFormat))
                .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

            const auto manager = el::LogManager::create();
            manager->setConfiguration(std::move(configuration));
            manager->rootStream()->info("Expedition clock synchronized."_el);
            manager->shutdown();
        };

        showZone("UTC rendering"_el, el::LogTimestampZone::Utc);
        showZone("Local rendering"_el, el::LogTimestampZone::Local);
    }

.. erbsland-ansi::
    :escape-char: ␛

    UTC rendering:
    ␛[96m2026-09-04 18:40:07Z␛[97m - Expedition clock synchronized.
    ␛[39mLocal rendering:
    ␛[96m2026-09-04 20:40:07+02:00␛[97m - Expedition clock synchronized.␛[0m

.. erbsland-demo-end::

Choose Compact or Descriptive Level Names
=========================================

A severity can be terse in a dense terminal view or deliberately conspicuous in a report.
:cpp:func:`LogLineFormat::setLevelFormat <erbsland::log::LogLineFormat::setLevelFormat>` controls the spelling inserted
for ``{level}``: ``ThreeLetterUpper`` produces values such as ``WRN`` and is the default, ``ShortLower`` produces
``wrn``, ``FullLower`` produces ``warning``, and ``FullUpper`` produces ``WARNING``.

Only the rendered spelling changes.
The entry remains a warning in all four cases, so writer routing and the console writer's semantic styling continue to
use the original severity.
This makes it safe to choose a format for readability without changing which entries reach a destination.

.. erbsland-demo::
    :source: log/LoggingTopics/LineLevelFormats.cpp
    :exec: log/logging_topics --demo LineLevelFormats
    :source-sha256: 10d1ded59ffb524925da45d184305da48b45e18b2f150e9490281468d07af250

.. code-block:: cpp

    /// Choose a compact or descriptive spelling for severity levels.
    ///
    /// `setLevelFormat()` affects only the text inserted by `{level}`. The entry keeps the same warning level, so routing
    /// and console styling remain unchanged across all four representations.
    void lineLevelFormats() {
        const auto showLevel = [](const el::String &label, const el::LogLevelFormat levelFormat) -> void {
            el::io::printLine(label, ":"_el);
            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern("{level}: {message}"_el).setLevelFormat(levelFormat);
            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(lineFormat))
                .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

            const auto manager = el::LogManager::create();
            manager->setConfiguration(std::move(configuration));
            manager->rootStream()->warn("One route marker needs repainting."_el);
            manager->shutdown();
        };

        showLevel("Three uppercase letters"_el, el::LogLevelFormat::ThreeLetterUpper);
        showLevel("Three lowercase letters"_el, el::LogLevelFormat::ShortLower);
        showLevel("Full lowercase name"_el, el::LogLevelFormat::FullLower);
        showLevel("Full uppercase name"_el, el::LogLevelFormat::FullUpper);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Three uppercase letters:
    ␛[93mWRN: One route marker needs repainting.
    ␛[39mThree lowercase letters:
    ␛[93mwrn: One route marker needs repainting.
    ␛[39mFull lowercase name:
    ␛[93mwarning: One route marker needs repainting.
    ␛[39mFull uppercase name:
    ␛[93mWARNING: One route marker needs repainting.␛[0m

.. erbsland-demo-end::

Show the Useful Part of a Stream Name
=====================================

Hierarchical stream paths are most valuable when they reveal where an entry originated, but the most useful portion
depends on the surrounding output.
Call :cpp:func:`LogLineFormat::setNameFormat <erbsland::log::LogLineFormat::setNameFormat>` to select the representation
inserted for ``{name}``.

``Full`` is the default and preserves the complete path.
``Leaf`` keeps only its final segment, which works well when an application has already separated output by subsystem.
``HeadAndLeaf`` combines the first and final segments with ``/…/``; it retains both the broad domain and the specific
producer while discarding intermediate organization.
``LeftTruncated`` preserves as much of the right side as fits within the name limit, where the most specific segments
are usually found.

The following output uses the same stream for every entry, making the information retained by each choice easy to
compare.

.. erbsland-demo::
    :source: log/LoggingTopics/LineNameFormats.cpp
    :exec: log/logging_topics --demo LineNameFormats
    :source-sha256: e13ba2f43b9c9747ef7f8f86a1fb9340989dc71b6252d4145bee9a292e9e89f2

.. code-block:: cpp

    /// Select how much of a hierarchical stream path appears in `{name}`.
    ///
    /// `setNameFormat()` can preserve the full path, keep its leaf, combine its first and final segments, or retain the
    /// right side of a long path within the default 40-character name limit.
    void lineNameFormats() {
        const auto showName = [](const el::String &label, const el::LogNameFormat nameFormat) -> void {
            el::io::printLine(label, ":"_el);
            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern("{name}: {message}"_el).setNameFormat(nameFormat);
            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(lineFormat))
                .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

            const auto manager = el::LogManager::create();
            manager->setConfiguration(std::move(configuration));
            manager->createStream("guild/exploration/finland/lapland/northern-lights"_el)->info("Survey opened."_el);
            manager->shutdown();
        };

        showName("Full path"_el, el::LogNameFormat::Full);
        showName("Leaf segment"_el, el::LogNameFormat::Leaf);
        showName("First and final segments"_el, el::LogNameFormat::HeadAndLeaf);
        showName("Left-truncated path"_el, el::LogNameFormat::LeftTruncated);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Full path:
    ␛[95mguild/exploration/finland/lapland/northern-lights␛[97m: Survey opened.
    ␛[39mLeaf segment:
    ␛[95mnorthern-lights␛[97m: Survey opened.
    ␛[39mFirst and final segments:
    ␛[95mguild/…/northern-lights␛[97m: Survey opened.
    ␛[39mLeft-truncated path:
    ␛[95m…oration/finland/lapland/northern-lights␛[97m: Survey opened.␛[0m

.. erbsland-demo-end::

Bound Left-Truncated Stream Names
=================================

The ``LeftTruncated`` representation becomes especially useful when stream hierarchies can grow with plugins, jobs, or
other runtime data.
:cpp:func:`LogLineFormat::setNameLimit <erbsland::log::LogLineFormat::setNameLimit>` sets its maximum length in Unicode
code points.
The default is 40 code points.
The formatter keeps the right side and places the configured truncation mark at the beginning, so a reader can see both
that context was removed and which specific producer emitted the entry.

The name limit is consulted only when ``LogNameFormat::LeftTruncated`` is selected.
It does not silently shorten ``Full``, ``Leaf``, or ``HeadAndLeaf`` names.
A limit of zero disables shortening and therefore shows the complete path even in ``LeftTruncated`` mode.

.. erbsland-demo::
    :source: log/LoggingTopics/LineNameLimits.cpp
    :exec: log/logging_topics --demo LineNameLimits
    :source-sha256: 71f578fdcf592f787f30e67342bff9152296c31ffa1d94c67438f0f6786a49aa

.. code-block:: cpp

    /// Bound left-truncated stream names by a code-point count.
    ///
    /// `setNameLimit()` is consulted only for `LogNameFormat::LeftTruncated`. Keeping the right side preserves the most
    /// specific path segments; a zero limit disables name shortening even when that format is selected.
    void lineNameLimits() {
        const auto showLimit = [](const el::String &label, const el::CpLength limit) -> void {
            el::io::printLine(label, ":"_el);
            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern("{name}: {message}"_el)
                .setNameFormat(el::LogNameFormat::LeftTruncated)
                .setNameLimit(limit);
            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(lineFormat))
                .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

            const auto manager = el::LogManager::create();
            manager->setConfiguration(std::move(configuration));
            manager->createStream("guild/exploration/finland/lapland/northern-lights"_el)->info("Survey opened."_el);
            manager->shutdown();
        };

        showLimit("32 characters"_el, el::CpLength{32U});
        showLimit("18 characters"_el, el::CpLength{18U});
        showLimit("Unlimited"_el, el::CpLength::zero());
    }

.. erbsland-ansi::
    :escape-char: ␛

    32 characters:
    ␛[95m…finland/lapland/northern-lights␛[97m: Survey opened.
    ␛[39m18 characters:
    ␛[95m…d/northern-lights␛[97m: Survey opened.
    ␛[39mUnlimited:
    ␛[95mguild/exploration/finland/lapland/northern-lights␛[97m: Survey opened.␛[0m

.. erbsland-demo-end::

Keep Multiline Messages or Only Their First Line
================================================

Multiline messages are useful for structured explanations and diagnostics, yet they can be awkward in destinations where
one physical line is expected to represent one event.
:cpp:func:`LogLineFormat::setMessageTruncation <erbsland::log::LogLineFormat::setMessageTruncation>` chooses how the
message is prepared before it is inserted into the pattern.

``LogMessageTruncation::None`` is the default and preserves the complete message, including line breaks.
``LogMessageTruncation::FirstLine`` stops at the first newline and appends the truncation mark when later lines were
removed.
This mode does not consult the numeric message limit: its boundary is the first line, not a character count.

.. erbsland-demo::
    :source: log/LoggingTopics/LineFirstLineTruncation.cpp
    :exec: log/logging_topics --demo LineFirstLineTruncation
    :source-sha256: 56492a81d4af60095635a1b2633f0f313ef8490ec4bfe0560f92982570c80156

.. code-block:: cpp

    /// Choose whether multiline messages remain complete or stop after their first line.
    ///
    /// `LogMessageTruncation::None` is the default and preserves line breaks. `FirstLine` keeps the text before the first
    /// newline and appends the configured truncation mark without consulting the message limit.
    void lineFirstLineTruncation() {
        const auto showMessage = [](const el::String &label, const el::LogMessageTruncation truncation) -> void {
            el::io::printLine(label, ":"_el);
            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern("{level}: {message}"_el).setMessageTruncation(truncation);
            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(lineFormat))
                .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

            const auto manager = el::LogManager::create();
            manager->setConfiguration(std::move(configuration));
            manager->rootStream()->info("Cloud cover is increasing.\nThe ridge team will wait below the summit."_el);
            manager->shutdown();
        };

        showMessage("Complete message"_el, el::LogMessageTruncation::None);
        showMessage("First line only"_el, el::LogMessageTruncation::FirstLine);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Complete message:
    ␛[97mINF: Cloud cover is increasing.
    The ridge team will wait below the summit.
    ␛[39mFirst line only:
    ␛[97mINF: Cloud cover is increasing.…␛[0m

.. erbsland-demo-end::

Limit the Message by Character Count
====================================

When the message itself must fit into a predictable space, select ``LogMessageTruncation::CharacterCount`` and set the
budget with :cpp:func:`LogLineFormat::setMessageLimit <erbsland::log::LogLineFormat::setMessageLimit>`.
The limit counts Unicode code points in the message, rather than encoded bytes.
Pattern literals, the level, the timestamp, and the stream name sit outside this particular budget.

The truncation mark occupies part of the available count, keeping the final message within the requested length.
A zero limit disables character-count shortening instead of producing an empty message.
This makes it convenient to expose ``0`` as an application setting meaning “unlimited.”

.. erbsland-demo::
    :source: log/LoggingTopics/LineMessageLimits.cpp
    :exec: log/logging_topics --demo LineMessageLimits
    :source-sha256: 2482d52b1f986eb75075e733829f98b06a605878014433564d986892b2a20adc

.. code-block:: cpp

    /// Limit the rendered message while leaving the other pattern fields outside the budget.
    ///
    /// Select `CharacterCount` with `setMessageTruncation()`, then pass the code-point budget to `setMessageLimit()`. A
    /// zero message limit disables this shortening rule.
    void lineMessageLimits() {
        const auto showLimit = [](const el::String &label, const el::CpLength limit) -> void {
            el::io::printLine(label, ":"_el);
            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern("{level} [{name}] {message}"_el)
                .setMessageTruncation(el::LogMessageTruncation::CharacterCount)
                .setMessageLimit(limit);
            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(lineFormat))
                .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

            const auto manager = el::LogManager::create();
            manager->setConfiguration(std::move(configuration));
            manager->createStream("guild/weather"_el)
                ->warn("A snow front will reach the northern platform before dusk."_el);
            manager->shutdown();
        };

        showLimit("48 message characters"_el, el::CpLength{48U});
        showLimit("24 message characters"_el, el::CpLength{24U});
        showLimit("Unlimited message"_el, el::CpLength::zero());
    }

.. erbsland-ansi::
    :escape-char: ␛

    48 message characters:
    ␛[93mWRN [␛[95mguild/weather␛[93m] A snow front will reach the northern platform b…
    ␛[39m24 message characters:
    ␛[93mWRN [␛[95mguild/weather␛[93m] A snow front will reach…
    ␛[39mUnlimited message:
    ␛[93mWRN [␛[95mguild/weather␛[93m] A snow front will reach the northern platform before dusk.␛[0m

.. erbsland-demo-end::

Limit the Complete Rendered Line
================================

Sometimes the destination, rather than the message, imposes the useful boundary.
``LogMessageTruncation::TotalLineLength`` treats
:cpp:func:`LogLineFormat::setMessageLimit <erbsland::log::LogLineFormat::setMessageLimit>` as a code-point budget for the
complete rendered line.
The formatter first accounts for pattern literals and the rendered time, level, and name, then gives the remaining space
to ``{message}``.
A detailed prefix therefore leaves less room for the same message, as the demo illustrates.

If ``{message}`` occurs more than once, its remaining budget is shared between those occurrences.
If fixed fields already consume the complete budget, the message becomes empty; fixed fields themselves are never
shortened by this setting.
As with character-count mode, a zero limit disables the rule.

.. erbsland-demo::
    :source: log/LoggingTopics/LineTotalLimits.cpp
    :exec: log/logging_topics --demo LineTotalLimits
    :source-sha256: 91393a5fec4dee7e40f2325db3e7423dda1d5819385768294a5e9a92a385dda0

.. code-block:: cpp

    /// Limit the complete rendered line by giving the message the space left by fixed fields.
    ///
    /// `TotalLineLength` counts pattern literals, the level, the rendered name, and every message occurrence against the
    /// `setMessageLimit()` budget. A richer prefix therefore leaves fewer characters for the message.
    void lineTotalLimits() {
        const auto showPattern = [](const el::String &label, el::String pattern) -> void {
            el::io::printLine(label, ":"_el);
            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern(std::move(pattern))
                .setMessageTruncation(el::LogMessageTruncation::TotalLineLength)
                .setMessageLimit(el::CpLength{56U});
            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(lineFormat))
                .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

            const auto manager = el::LogManager::create();
            manager->setConfiguration(std::move(configuration));
            manager->createStream("guild/weather"_el)
                ->warn("A snow front will reach the northern platform before dusk."_el);
            manager->shutdown();
        };

        showPattern("Message-only pattern"_el, "{message}"_el);
        showPattern("Level-and-name pattern"_el, "{level} [{name}] {message}"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Message-only pattern:
    ␛[93mA snow front will reach the northern platform before du…
    ␛[39mLevel-and-name pattern:
    ␛[93mWRN [␛[95mguild/weather␛[93m] A snow front will reach the norther…␛[0m

.. erbsland-demo-end::

Choose a Clear Truncation Mark
==============================

A shortened value should look shortened.
The default mark is the single ellipsis character ``…``, a compact choice that works well in ordinary console and file
output.
:cpp:func:`LogLineFormat::setTruncationMark <erbsland::log::LogLineFormat::setTruncationMark>` can replace it with text
that better suits a plain-text destination, or with an empty string when the surrounding format already makes the limit
obvious.

The same mark is used for first-line messages, message character limits, complete-line limits, and left-truncated names.
For the numeric limits it is part of the code-point budget, so a longer marker leaves less room for retained text.
An empty marker saves that space, but it also removes the reader's visible clue that text is missing.

.. erbsland-demo::
    :source: log/LoggingTopics/LineTruncationMarks.cpp
    :exec: log/logging_topics --demo LineTruncationMarks
    :source-sha256: 427ae1660db7207b4eaa2944cb08ebfba010d568e5951ee6f474a5a18724ca58

.. code-block:: cpp

    /// Choose the visible mark that tells readers text was shortened.
    ///
    /// `setTruncationMark()` replaces the default ellipsis for first-line, character-count, total-line, and left-truncated
    /// name formatting. The mark occupies part of a character-count budget and may also be empty.
    void lineTruncationMarks() {
        const auto showMark = [](const el::String &label, el::String mark) -> void {
            el::io::printLine(label, ":"_el);
            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern("{message}"_el)
                .setMessageTruncation(el::LogMessageTruncation::CharacterCount)
                .setMessageLimit(el::CpLength{36U})
                .setTruncationMark(std::move(mark));
            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(lineFormat))
                .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

            const auto manager = el::LogManager::create();
            manager->setConfiguration(std::move(configuration));
            manager->rootStream()->info("The northern observation platform is closing for the night."_el);
            manager->shutdown();
        };

        showMark("Ellipsis"_el, "…"_el);
        showMark("Text marker"_el, " [more]"_el);
        showMark("No marker"_el, ""_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Ellipsis:
    ␛[97mThe northern observation platform i…
    ␛[39mText marker:
    ␛[97mThe northern observation plat [more]
    ␛[39mNo marker:
    ␛[97mThe northern observation platform is␛[0m

.. erbsland-demo-end::

Keep Producer Byte Limits Separate
==================================

All limits on :cpp:class:`LogLineFormat <erbsland::log::LogLineFormat>` are presentation choices applied when a writer
renders an entry.
They do not reduce the message retained in the manager's queue, and their units are Unicode code points.
Choose them to keep logs pleasant to scan and compatible with a destination's line conventions.

:cpp:func:`LogManagerOptions::setMaximumMessageBytes <erbsland::log::LogManagerOptions::setMaximumMessageBytes>` solves a
different problem.
It places a producer-side byte limit on the sanitized message before the entry enters the queue, protecting memory even
when a writer later uses an unlimited line format.
Configure that boundary as part of :doc:`configuring_manager_options`, independently of the line layout readers see.
