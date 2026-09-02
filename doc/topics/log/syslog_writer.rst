.. index::
    single: Logging; Syslog Writer
    single: Log Writers; Syslog
    single: Syslog; RFC 5424
    single: Syslog; UDP
    single: Syslog; TCP
    single: Syslog; TLS

**********************
Sending Logs to Syslog
**********************

The ``LogWriter::createForSyslog()`` sends operational records to a syslog collector in RFC 5424 format.
Instead of leaving every service's history on the machine that produced it, syslog gives a deployment one place to
search, retain, alert on, and correlate events from many processes or hosts.
It is a natural destination for background services and managed installations where operators already depend on a
central logging system.

Remote logging introduces choices that a local file does not have.
You must select a transport and endpoint, give the collector enough stable identity to classify each message, decide
which RFC facility represents the application, and bound the data waiting during network pressure.
TLS adds one more connection to the application's reusable network security policy.

This page builds a typical syslog route and then explains every option on
:cpp:class:`SyslogLogWriterOptions <erbsland::log::SyslogLogWriterOptions>`.
It also distinguishes drops inside the syslog writer from drops in the manager's producer queue.
For deciding which levels and stream paths should be sent remotely, start with :doc:`using_writers`.

Add a Syslog Writer to the Configuration
========================================

Create :cpp:class:`SyslogLogWriterOptions <erbsland::log::SyslogLogWriterOptions>`, configure the remote destination and
RFC fields, and pass the completed value to ``LogWriter::createForSyslog()``.
Then add the shared writer to :cpp:class:`LogConfiguration <erbsland::log::LogConfiguration>` with a route filter.
Warnings and errors are often a good first remote route; high-volume information or trace traffic should be enabled only
after considering collector capacity and outage behavior.

Constructing the writer validates its options but does not open a connection.
Network activity begins when the manager delivers the first accepted entry.
That distinction lets the documentation demonstrate the real configuration API safely: the following example builds a
TLS writer and its warning/error route, then renders a deterministic RFC 5424 message and frame without installing the
configuration or contacting ``logs.example``.

.. erbsland-demo::
    :source: log/LoggingTopics/SyslogWriters.cpp
    :exec: log/logging_topics --demo SyslogWriters
    :source-sha256: 7e1b0c3f3484a03895aeb133c0886290924eae412a0718dc97d6f6e94cce8186

.. code-block:: cpp

    /// A syslog writer sends RFC 5424 messages over UDP, TCP, or TLS.
    ///
    /// The options describe the endpoint, facility, RFC header fields, TLS configuration label, and bounded pending data.
    /// A route can be assembled without opening a connection.
    void syslogWriters() {
        auto options = el::SyslogLogWriterOptions{};
        options.setTransport(el::SyslogTransport::Tls)
            .setEndpoint(el::HostEndpoint::fromStringOrThrow("logs.example:6514"_el))
            .setFacility(1U)
            .setHostName("guild-hall"_el)
            .setApplicationName("explorer-guild"_el)
            .setProcessId("314"_el)
            .setMessageId("route"_el)
            .setTlsConfigurationLabel("guild/syslog"_el)
            .setMaximumPendingBytes(el::ByteLength{256U * 1024U});

        auto configuration = el::LogConfiguration{};
        configuration.addWriter(
            el::LogWriter::createForSyslog(options),
            el::LogWriterFilter{el::LogLevels{el::LogLevel::Warning, el::LogLevel::Error}});

        el::io::printLine("Syslog endpoint: "_el, options.endpoint().toString());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Syslog endpoint: logs.example:6514

.. erbsland-demo-end::

Create and Pass the Options Object
==================================

:cpp:class:`SyslogLogWriterOptions <erbsland::log::SyslogLogWriterOptions>` is a complete value object.
Its default constructor produces a valid local UDP setup for ``127.0.0.1:514``, facility 1, application name
``erbsland-core``, and a 1 MiB pending-data limit.
Host name, process identifier, and message identifier initially use ``-``, the RFC 5424 NILVALUE.
The default TLS configuration label is ``log/syslog`` even though it is consulted only when TLS is selected.

These defaults are convenient for local C++ configuration, but a production deployment should state its collector and
identity deliberately.
After setting the required values, pass the options to ``LogWriter::createForSyslog()`` directly inside
``LogConfiguration::addWriter()``.
The writer keeps its own snapshot; changing the original object afterward cannot reconfigure an active destination.
To change syslog behavior, install a new complete log configuration with a newly constructed writer.

Choose UDP, TCP, or TLS Transport
=================================

:cpp:func:`setTransport() <erbsland::log::SyslogLogWriterOptions::setTransport>` chooses a
:cpp:class:`SyslogTransport <erbsland::log::SyslogTransport>`.
The default :cpp:enumerator:`Udp <erbsland::log::SyslogTransport::Udp>` sends each RFC 5424 message as one datagram.
It has little connection overhead and preserves message boundaries, but the transport provides no acknowledgement,
ordering across network loss, or retransmission guarantee.
Use it when the local network and collector are designed for conventional UDP syslog and occasional loss is accepted.

:cpp:enumerator:`Tcp <erbsland::log::SyslogTransport::Tcp>` uses a connected byte stream.
Because TCP has no message boundaries, the writer applies RFC 6587 octet-counted framing: the UTF-8 byte count, one
space, and then the complete RFC 5424 message.
The connection layer preserves order and detects connection failure, but it does not protect the content from a network
observer.

:cpp:enumerator:`Tls <erbsland::log::SyslogTransport::Tls>` uses the same octet-counted framing over a TLS client
connection.
It is the appropriate choice when logs cross an untrusted network or contain information that must be protected in
transit.
Certificate trust, peer verification, and related connection policy come from the TLS label described later on this
page.

The demo constructs one valid writer for each transport without delivering an entry.
It then shows the unframed UDP message beside the framed TCP and TLS forms.

.. erbsland-demo::
    :source: log/LoggingTopics/SyslogWriterOptions.cpp
    :function-blocks: syslogTransport
    :function-blocks-sha256: c9dc6a8c857af9f8d7eb8c9495bef75d2ed6f0f70d034f940e20c92c97654a46
    :exec: log/logging_topics --demo SyslogTransport
    :source-sha256: e8623b94ebda39515f14968f9c7963cbc4e814c1311fcc77c54b5eb5e58ae245

.. code-block:: cpp

    void syslogTransport() {
        auto udpOptions = el::SyslogLogWriterOptions{};
        udpOptions.setTransport(el::SyslogTransport::Udp)
            .setEndpoint(el::HostEndpoint::fromStringOrThrow("192.0.2.10:514"_el));

        auto tcpOptions = el::SyslogLogWriterOptions{};
        tcpOptions.setTransport(el::SyslogTransport::Tcp)
            .setEndpoint(el::HostEndpoint::fromStringOrThrow("logs.example:601"_el));

        auto tlsOptions = el::SyslogLogWriterOptions{};
        tlsOptions.setTransport(el::SyslogTransport::Tls)
            .setEndpoint(el::HostEndpoint::fromStringOrThrow("logs.example:6514"_el));

        auto configuration = el::LogConfiguration{};
        configuration.addWriter(el::LogWriter::createForSyslog(udpOptions))
            .addWriter(el::LogWriter::createForSyslog(tcpOptions))
            .addWriter(el::LogWriter::createForSyslog(tlsOptions));

        el::io::printLine("UDP endpoint: "_el, udpOptions.endpoint().toString());
        el::io::printLine("TCP endpoint: "_el, tcpOptions.endpoint().toString());
        el::io::printLine("TLS endpoint: "_el, tlsOptions.endpoint().toString());
    }

.. erbsland-ansi::
    :escape-char: ␛

    UDP endpoint: 192.0.2.10:514
    TCP endpoint: logs.example:601
    TLS endpoint: logs.example:6514

.. erbsland-demo-end::

Select the Collector Endpoint
=============================

:cpp:func:`setEndpoint() <erbsland::log::SyslogLogWriterOptions::setEndpoint>` accepts a
:cpp:class:`HostEndpoint <erbsland::network::HostEndpoint>` containing the collector host and transport port.
Build it directly from network value objects or parse familiar endpoint text with
:cpp:func:`HostEndpoint::fromStringOrThrow() <erbsland::network::HostEndpoint::fromStringOrThrow>`.
A zero or automatic port is rejected when the syslog writer is constructed.

UDP endpoints must contain a numeric IPv4 or IPv6 address.
The datagram writer does not perform host-name resolution, so ``192.0.2.10:514`` is valid for UDP while
``logs.example:514`` is not.
TCP and TLS endpoints may contain a numeric address or an unresolved host name; their connection layer resolves names as
part of connecting.

The C++ options default to the IPv4 loopback collector at ``127.0.0.1:514``.
ELCL intentionally requires an explicit endpoint so a deployed configuration cannot send remotely by accident or quietly
rely on a platform-specific local collector.
The following example shows the C++ default and a typical remote TLS endpoint.

.. erbsland-demo::
    :source: log/LoggingTopics/SyslogWriterOptions.cpp
    :function-blocks: syslogEndpoint
    :function-blocks-sha256: fa4c719b5da35a3e6f2b6204cd8c845c8c574107b5b178fda73b1d5d67b4f670
    :exec: log/logging_topics --demo SyslogEndpoint
    :source-sha256: e8623b94ebda39515f14968f9c7963cbc4e814c1311fcc77c54b5eb5e58ae245

.. code-block:: cpp

    void syslogEndpoint() {
        auto localOptions = el::SyslogLogWriterOptions{};

        auto remoteOptions = el::SyslogLogWriterOptions{};
        remoteOptions.setTransport(el::SyslogTransport::Tls)
            .setEndpoint(el::HostEndpoint::fromStringOrThrow("logs.example:6514"_el));

        auto configuration = el::LogConfiguration{};
        configuration.addWriter(el::LogWriter::createForSyslog(localOptions))
            .addWriter(el::LogWriter::createForSyslog(remoteOptions));

        el::io::printLine("Default endpoint: "_el, localOptions.endpoint().toString());
        el::io::printLine("Remote endpoint : "_el, remoteOptions.endpoint().toString());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Default endpoint: 127.0.0.1:514
    Remote endpoint : logs.example:6514

.. erbsland-demo-end::

Choose the RFC Facility
=======================

:cpp:func:`setFacility() <erbsland::log::SyslogLogWriterOptions::setFacility>` selects an RFC 5424 facility number from
zero through 23. The default is 1, traditionally the user-level facility.
A value above 23 is rejected immediately by the setter.
Choose the value expected by the collector's routing policy; facilities 16 through 23 are commonly reserved for local
use, but their exact meaning belongs to the deployment.

Syslog combines facility and severity into the priority at the beginning of every message.
The writer maps trace to severity 7, information to 6, warning to 4, and error to 3, then calculates
``facility * 8 + severity``.
The application continues to log with ordinary :cpp:class:`LogLevel <erbsland::log::LogLevel>` values; no syslog numbers
enter module code.

The fixed warning in this demo produces priority 12 with facility 1 and priority 132 with local-use facility 16.

.. erbsland-demo::
    :source: log/LoggingTopics/SyslogWriterOptions.cpp
    :function-blocks: syslogFacility
    :function-blocks-sha256: 91a93f1d717874201f80626ac62282c7e747f225e7a6b2efd34a5cabd10ccd69
    :exec: log/logging_topics --demo SyslogFacility
    :source-sha256: e8623b94ebda39515f14968f9c7963cbc4e814c1311fcc77c54b5eb5e58ae245

.. code-block:: cpp

    void syslogFacility() {
        auto userOptions = el::SyslogLogWriterOptions{};
        userOptions.setFacility(1U);
        auto localOptions = el::SyslogLogWriterOptions{};
        localOptions.setFacility(16U);

        el::io::printLine("Facility 1 : "_el, userOptions.facility());
        el::io::printLine("Facility 16: "_el, localOptions.facility());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Facility 1 : 1
    Facility 16: 16

.. erbsland-demo-end::

Identify the Producing Host
===========================

:cpp:func:`setHostName() <erbsland::log::SyslogLogWriterOptions::setHostName>` fills the RFC 5424 ``HOSTNAME`` field.
Its default is ``-``.
Set a stable machine identity when the collector cannot add one reliably from the transport or when messages from many
hosts share the same destination.
Avoid a transient display name that changes independently of the machine operators recognize.

The field must be nonempty printable ASCII, no longer than 255 characters, and must not contain ``=``, ``]``, or a
double quote.
A literal ``-`` is valid and represents NILVALUE.
Header fields are validated together when the writer is constructed, so finish all option changes before creating the
writer.

.. erbsland-demo::
    :source: log/LoggingTopics/SyslogWriterOptions.cpp
    :function-blocks: syslogHostName
    :function-blocks-sha256: 540810225e2721a10a11436cca8846eb07c912aefb4f64026e8d97dfc15259e0
    :exec: log/logging_topics --demo SyslogHostName
    :source-sha256: e8623b94ebda39515f14968f9c7963cbc4e814c1311fcc77c54b5eb5e58ae245

.. code-block:: cpp

    void syslogHostName() {
        auto options = el::SyslogLogWriterOptions{};
        options.setHostName("guild-hall"_el);
        el::io::printLine(options.hostName());
    }

.. erbsland-ansi::
    :escape-char: ␛

    guild-hall

.. erbsland-demo-end::

Name the Application
====================

:cpp:func:`setApplicationName() <erbsland::log::SyslogLogWriterOptions::setApplicationName>` fills ``APP-NAME``.
The default is ``erbsland-core``.
A concise service name helps operators group messages across process restarts and across hosts running the same
application.
Keep it stable across versions unless the collector intentionally treats those versions as different sources.

The application name follows the same printable-ASCII restrictions as the other header fields and may contain at most 48
characters.
Use ``-`` when the application truly has no useful identity.

.. erbsland-demo::
    :source: log/LoggingTopics/SyslogWriterOptions.cpp
    :function-blocks: syslogApplicationName
    :function-blocks-sha256: 0fd3f848752e25168fcb7b0fd4460ed086de4d167a1944b86c3f2500ba66d58c
    :exec: log/logging_topics --demo SyslogApplicationName
    :source-sha256: e8623b94ebda39515f14968f9c7963cbc4e814c1311fcc77c54b5eb5e58ae245

.. code-block:: cpp

    void syslogApplicationName() {
        auto options = el::SyslogLogWriterOptions{};
        options.setApplicationName("explorer-guild"_el);
        el::io::printLine(options.applicationName());
    }

.. erbsland-ansi::
    :escape-char: ␛

    explorer-guild

.. erbsland-demo-end::

Identify the Process Instance
=============================

:cpp:func:`setProcessId() <erbsland::log::SyslogLogWriterOptions::setProcessId>` fills ``PROCID``.
Its default is ``-``.
A numeric operating-system process identifier is a common value, but the field may be any stable printable ASCII token
that distinguishes concurrent instances.
If instance identity is already carried elsewhere and adds no operational value, leaving NILVALUE is clearer than
inventing one.

The process identifier may contain at most 128 characters and follows the same excluded-character rules as ``HOSTNAME``.

.. erbsland-demo::
    :source: log/LoggingTopics/SyslogWriterOptions.cpp
    :function-blocks: syslogProcessId
    :function-blocks-sha256: ddf2ed727dce20de124809ccc5c5d9e3f839aa645c32e01889be518182c84fd4
    :exec: log/logging_topics --demo SyslogProcessId
    :source-sha256: e8623b94ebda39515f14968f9c7963cbc4e814c1311fcc77c54b5eb5e58ae245

.. code-block:: cpp

    void syslogProcessId() {
        auto options = el::SyslogLogWriterOptions{};
        options.setProcessId("314"_el);
        el::io::printLine(options.processId());
    }

.. erbsland-ansi::
    :escape-char: ␛

    314

.. erbsland-demo-end::

Classify the Message Family
===========================

:cpp:func:`setMessageId() <erbsland::log::SyslogLogWriterOptions::setMessageId>` fills ``MSGID``.
The default is ``-``.
This field works best as a stable event family such as ``startup``, ``route``, or ``database``, allowing collector rules
to recognize a category without parsing the human-readable message body.
It is not intended to hold a unique identifier or a copy of the changing message text.

The message identifier has the tightest header limit: 32 printable ASCII characters, excluding ``=``, ``]``, and a
double quote.

.. erbsland-demo::
    :source: log/LoggingTopics/SyslogWriterOptions.cpp
    :function-blocks: syslogMessageId
    :function-blocks-sha256: 9bd3a8a4688de4e4507b99ab00610d8e18ed9aba3276566a3f8dc617c8a28291
    :exec: log/logging_topics --demo SyslogMessageId
    :source-sha256: e8623b94ebda39515f14968f9c7963cbc4e814c1311fcc77c54b5eb5e58ae245

.. code-block:: cpp

    void syslogMessageId() {
        auto options = el::SyslogLogWriterOptions{};
        options.setMessageId("route"_el);
        el::io::printLine(options.messageId());
    }

.. erbsland-ansi::
    :escape-char: ␛

    route

.. erbsland-demo-end::

Understand the Resulting RFC Header
===================================

The four identity fields appear after the entry's UTC timestamp in this order: host name, application name, process
identifier, and message identifier.
The writer emits RFC version 1 and uses NILVALUE for structured data because no structured-data option is currently
defined.
The configured :cpp:class:`LogLine <erbsland::log::LogLine>` text then becomes the message body.

For reference, the complete shape is ``<priority>1 timestamp HOSTNAME APP-NAME PROCID MSGID - message``.
Keeping each header field stable and narrowly meaningful gives a collector useful structure while leaving the log line
free to remain readable to a person.

Select the TLS Configuration Label
==================================

:cpp:func:`setTlsConfigurationLabel() <erbsland::log::SyslogLogWriterOptions::setTlsConfigurationLabel>` chooses the
application-managed network/TLS policy used when transport is
:cpp:enumerator:`Tls <erbsland::log::SyslogTransport::Tls>`.
The default label is ``log/syslog``.
The label must be nonempty; an empty value is rejected by the setter.

The label is an indirection, not a certificate path or a bundle of TLS settings.
It lets one application define trust stores, peer verification, client identity, and related security policy in its
network configuration, then refer to that policy consistently from the log writer.
Changing the label has no effect for UDP or plain TCP.

The example selects a deployment-specific policy while constructing a valid TLS writer.
It does not open the connection because no log entry is delivered.

.. erbsland-demo::
    :source: log/LoggingTopics/SyslogWriterOptions.cpp
    :function-blocks: syslogTlsLabel
    :function-blocks-sha256: 122db55c171692302575aad7e6dddeca480702f88b5df2dfbf13b9e4dcd030c4
    :exec: log/logging_topics --demo SyslogTlsLabel
    :source-sha256: e8623b94ebda39515f14968f9c7963cbc4e814c1311fcc77c54b5eb5e58ae245

.. code-block:: cpp

    void syslogTlsLabel() {
        auto options = el::SyslogLogWriterOptions{};
        options.setTransport(el::SyslogTransport::Tls)
            .setEndpoint(el::HostEndpoint::fromStringOrThrow("logs.example:6514"_el))
            .setTlsConfigurationLabel("operations/syslog"_el);
        auto configuration = el::LogConfiguration{};
        configuration.addWriter(el::LogWriter::createForSyslog(options));

        el::io::printLine("TLS configuration label: "_el, options.tlsConfigurationLabel());
    }

.. erbsland-ansi::
    :escape-char: ␛

    TLS configuration label: operations/syslog

.. erbsland-demo-end::

Bound Pending Network Data
==========================

:cpp:func:`setMaximumPendingBytes() <erbsland::log::SyslogLogWriterOptions::setMaximumPendingBytes>` limits the encoded
data retained while the selected transport is connecting, temporarily unable to accept more data, or waiting for a
retry.
The default is 1 MiB, or 1,048,576 bytes.
The value must be positive.

The charge includes the UTF-8 RFC 5424 message and, for TCP or TLS, its octet-counting frame.
A new message is dropped when it cannot fit by itself or when adding it would push the current pending total beyond the
limit.
UDP also drops a datagram larger than 65,507 bytes even if the configured pending limit is higher, because that is the
writer's maximum UDP payload.
The writer never partially retains a syslog message.

The manager's delivery statistics report how many messages the logging pipeline has rejected.
That counter is separate from :cpp:member:`droppedEntries <erbsland::log::LogManagerStatistics::droppedEntries>`, which
describes entries rejected by the manager queue before writer delivery.
Monitor both when remote logs are important: one reveals producer pressure, the other reveals destination pressure.

This safe demo uses a limit smaller than one encoded message.
The message is rejected before any network transport is created, and the writer's counter becomes one.

.. erbsland-demo::
    :source: log/LoggingTopics/SyslogWriterOptions.cpp
    :function-blocks: syslogMaximumPendingBytes
    :function-blocks-sha256: b8e9825434543e1148ed88eeb5d21a18e5119634349462171723ce6a3496849b
    :exec: log/logging_topics --demo SyslogMaximumPendingBytes
    :source-sha256: e8623b94ebda39515f14968f9c7963cbc4e814c1311fcc77c54b5eb5e58ae245

.. code-block:: cpp

    void syslogMaximumPendingBytes() {
        auto options = el::SyslogLogWriterOptions{};
        options.setMaximumPendingBytes(el::ByteLength{32U});
        auto configuration = el::LogConfiguration{};
        configuration.addWriter(el::LogWriter::createForSyslog(options));
        el::io::printLine("Maximum pending bytes: "_el, options.maximumPendingBytes());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Maximum pending bytes: 32

.. erbsland-demo-end::

Plan for Collector and Network Failures
=======================================

TCP and TLS failures close the active transport and schedule a reconnect.
The delay grows from one second to a maximum of 32 seconds while pending messages remain bounded by the configured byte
limit.
Application threads do not perform this connection work; they continue to submit entries through the log manager.
UDP has no delivery session and therefore cannot confirm that a datagram reached the collector.

No transport turns syslog into guaranteed storage.
A process can exit with pending data, a network can lose UDP datagrams, and a prolonged outage can exhaust the pending
limit.
When remote diagnostics are essential for recovering the same host, add a local ``LogWriter::createForFile()`` route as
well.
The file provides durable local history, while syslog provides central visibility; the two destinations solve different
failure cases and work well together.
