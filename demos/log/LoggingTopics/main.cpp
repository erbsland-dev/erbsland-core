// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LoggingTopicsDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.enableTerminal();
    app.registerDemo("ApplicationLogging"_el, applicationLogging);
    app.registerDemo("ConsoleWriterBaseStyle"_el, consoleWriterBaseStyle);
    app.registerDemo("ConsoleWriterLevelPartStyles"_el, consoleWriterLevelPartStyles);
    app.registerDemo("ConsoleWriterLevelStyles"_el, consoleWriterLevelStyles);
    app.registerDemo("ConsoleWriterParagraphOptions"_el, consoleWriterParagraphOptions);
    app.registerDemo("ConsoleWriterPartStyles"_el, consoleWriterPartStyles);
    app.registerDemo("ConsoleWriters"_el, consoleWriters);
    app.registerDemo("ConfigurationReplacement"_el, configurationReplacement);
    app.registerDemo("FileWriterMaximumSize"_el, fileWriterMaximumSize);
    app.registerDemo("FileWriterModes"_el, fileWriterModes);
    app.registerDemo("FileWriterRetention"_el, fileWriterRetention);
    app.registerDemo("FileWriterRotation"_el, fileWriterRotation);
    app.registerDemo("FileWriters"_el, fileWriters);
    app.registerDemo("LastErrorsWriter"_el, lastErrorsWriter);
    app.registerDemo("LineFirstLineTruncation"_el, lineFirstLineTruncation);
    app.registerDemo("LineFormats"_el, lineFormats);
    app.registerDemo("LineLevelFormats"_el, lineLevelFormats);
    app.registerDemo("LineMessageLimits"_el, lineMessageLimits);
    app.registerDemo("LineNameFormats"_el, lineNameFormats);
    app.registerDemo("LineNameLimits"_el, lineNameLimits);
    app.registerDemo("LinePatterns"_el, linePatterns);
    app.registerDemo("LineTimestampZones"_el, lineTimestampZones);
    app.registerDemo("LineTotalLimits"_el, lineTotalLimits);
    app.registerDemo("LineTruncationMarks"_el, lineTruncationMarks);
    app.registerDemo("LoadApplicationLogConfiguration"_el, loadApplicationLogConfiguration);
    app.registerDemo("LoadStandaloneLogConfiguration"_el, loadStandaloneLogConfiguration);
    app.registerDemo("LogLevels"_el, logLevels);
    app.registerDemo("LogStreams"_el, logStreams);
    app.registerDemo("ManagerByteCapacity"_el, managerByteCapacity);
    app.registerDemo("ManagerEntryCapacity"_el, managerEntryCapacity);
    app.registerDemo("ManagerErrorByteReserve"_el, managerErrorByteReserve);
    app.registerDemo("ManagerErrorEntryReserve"_el, managerErrorEntryReserve);
    app.registerDemo("ManagerMessageSize"_el, managerMessageSize);
    app.registerDemo("ManagerOptions"_el, managerOptions);
    app.registerDemo("ManagerShutdownTimeout"_el, managerShutdownTimeout);
    app.registerDemo("ManagerStatistics"_el, managerStatistics);
    app.registerDemo("ManualConfiguration"_el, manualConfiguration);
    app.registerDemo("MultipleLogStreams"_el, multipleLogStreams);
    app.registerDemo("StoredLogStream"_el, storedLogStream);
    app.registerDemo("SyslogApplicationName"_el, syslogApplicationName);
    app.registerDemo("SyslogEndpoint"_el, syslogEndpoint);
    app.registerDemo("SyslogFacility"_el, syslogFacility);
    app.registerDemo("SyslogHostName"_el, syslogHostName);
    app.registerDemo("SyslogMaximumPendingBytes"_el, syslogMaximumPendingBytes);
    app.registerDemo("SyslogMessageId"_el, syslogMessageId);
    app.registerDemo("SyslogProcessId"_el, syslogProcessId);
    app.registerDemo("SyslogTlsLabel"_el, syslogTlsLabel);
    app.registerDemo("SyslogTransport"_el, syslogTransport);
    app.registerDemo("SyslogWriters"_el, syslogWriters);
    app.registerDemo("TraceSectionActivation"_el, traceSectionActivation);
    app.registerDemo("TraceSectionGrouping"_el, traceSectionGrouping);
    app.registerDemo("TraceSectionGuard"_el, traceSectionGuard);
    app.registerDemo("TraceSectionReplacement"_el, traceSectionReplacement);
    app.registerDemo("TraceSections"_el, traceSections);
    app.registerDemo("WriterRouting"_el, writerRouting);
    app.registerDemo("WriterSetup"_el, writerSetup);
    return app.run();
}
