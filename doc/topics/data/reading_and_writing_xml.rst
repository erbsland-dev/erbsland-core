..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: XML; Reading and Writing
    single: XML; Parsing
    single: XML; Serialization
    single: XML; Entity References

***********************
Reading and Writing XML
***********************

XML is useful when a protocol or document format needs named elements, attributes, and text in a defined order.
This page shows how to parse an XML document, build and serialize one, and navigate its nodes.
It also explains what happens to mixed content and entity references, and how to set limits when XML comes from an
untrusted source.

What Is an XML Document?
========================

An XML document has one root element.
Elements can contain text and other elements, while attributes attach named text values to an element.
A document can also have an XML declaration, a DOCTYPE declaration, comments, or processing instructions around its
root.
Inside an element, the order matters: in ``<gruppe>Solo: <instrument>fløjte</instrument> og obo</gruppe>``, the words
before and after ``instrument`` are separate pieces of text.
That makes XML suitable for both structured records and documents with mixed prose and markup.

Erbsland Core represents an XML document with :cpp:class:`XmlDocument <erbsland::data::xml::XmlDocument>` and its
ordered :cpp:class:`XmlNode <erbsland::data::xml::XmlNode>` objects.
This is a document tree rather than a collection of typed scalar values: numbers, dates, and booleans in XML are text
until your application interprets them.
The document interface uses :cpp:type:`String <erbsland::text::String>` for XML text, and the serializer returns a
``String``.
When an application works with bytes from a file or network connection, it must decode them into a ``String`` before
calling the XML parser and encode the resulting ``String`` for its destination.

The implementation retains qualified names such as ``musik:orkester`` and namespace declarations such as ``xmlns:musik``
as written.
It does not resolve prefixes to namespace URIs or validate namespace bindings.
If your protocol relies on namespaces, check the relevant names and declarations in the parsed document according to
that protocol.
The parser preserves XML and DOCTYPE declarations but does not use them to decode bytes or validate a DTD or schema.

Parsing XML Text
================

Pass one complete document to :cpp:func:`XmlDocument::fromString <erbsland::data::xml::XmlDocument::fromString>`.
It returns an optional document, which is convenient when invalid input is an expected validation result.
Use :cpp:func:`fromStringOrThrow <erbsland::data::xml::XmlDocument::fromStringOrThrow>` when you need a diagnostic;
malformed or unsupported input raises :cpp:class:`ParseError <erbsland::err::ParseError>`.
Both forms require exactly one root element and reject malformed nesting, duplicate attributes, and non-whitespace text
outside the root.

This example receives an orchestra document, reads a group attribute, then demonstrates the throwing form on mismatched
end tags.

.. erbsland-demo::
    :source: data/DataFormats/XmlExamples.cpp
    :function-blocks: parseXmlDocument
    :function-blocks-sha256: 90b01ff730063f5b89def3e70d12555888e8b4fd7531202e5f35097eeb9cd9b1
    :exec: data/data_formats --demo ParseXmlDocument
    :source-sha256: a145738dffc0ee7977bd43f71e5b85f85645263c0897e7e4c042b1dd1b60911b

.. code-block:: cpp

    void parseXmlDocument() {
        // Accept a complete document and read a value from its first child element.
        const auto source = "<orkester><gruppe navn=\"strygere\"><instrument>violin</instrument></gruppe></orkester>"_el;
        if (const auto document = XmlDocument::fromString(source)) {
            el::io::printLine("Root: "_el, document->root()->name());
            el::io::printLine(
                "Group: "_el, document->root()->children().get(el::ItemIndex{0U})->getAttribute("navn"_el).value());
        }
        // Use the throwing form when the caller needs a parse diagnostic.
        try {
            (void)XmlDocument::fromStringOrThrow("<orkester><gruppe></orkester>"_el);
        } catch (const el::err::ParseError &) {
            el::io::printLine("Invalid XML document"_el);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Root: orkester
    Group: strygere
    Invalid XML document

.. erbsland-demo-end::

Building a Document
===================

Start with an empty ``XmlDocument`` and call :cpp:func:`addRoot <erbsland::data::xml::XmlDocument::addRoot>` once.
On an element, :cpp:func:`addElement <erbsland::data::xml::XmlNode::addElement>` appends a child and returns it, so you
can build the next level immediately.
Its optional text argument creates a text child when nonempty.
Use :cpp:func:`setAttribute <erbsland::data::xml::XmlNode::setAttribute>` for an attribute: calling it again with the
same name replaces the value in its existing position.
Attributes are text, even if your application uses them for quantities or identifiers.

The orchestra below groups instruments under the root.
The child order is the order in which each ``addElement()`` call appears.

.. erbsland-demo::
    :source: data/DataFormats/XmlExamples.cpp
    :function-blocks: buildXmlDocument
    :function-blocks-sha256: 042165b407949ce77fcc6eb15918ef57013d4b1b8a506a547550bdb08b5cb1e0
    :exec: data/data_formats --demo BuildXmlDocument
    :source-sha256: a145738dffc0ee7977bd43f71e5b85f85645263c0897e7e4c042b1dd1b60911b

.. code-block:: cpp

    void buildXmlDocument() {
        // Append groups and instruments in the order they should appear in XML.
        auto document = XmlDocument{};
        auto root = document.addRoot("orkester"_el);
        root->setAttribute("sted"_el, "Aarhus"_el);
        auto group = root->addElement("gruppe"_el);
        group->setAttribute("navn"_el, "strygere"_el);
        group->addElement("instrument"_el, "violin"_el);
        group->addElement("instrument"_el, "cello"_el);
        el::io::printLine("Groups: "_el, root->children().count().toSizeT());
        el::io::printLine("Instruments: "_el, group->children().count().toSizeT());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Groups: 1
    Instruments: 2

.. erbsland-demo-end::

When you need XML markup beyond plain elements and attributes, create a node with the appropriate ``XmlNode`` factory
and place it with :cpp:func:`XmlDocument::add <erbsland::data::xml::XmlDocument::add>` or
:cpp:func:`XmlNode::add <erbsland::data::xml::XmlNode::add>`.
A declaration must be the first document node, and a DOCTYPE belongs before the root.
``createDeclaration()`` and ``createDocType()`` take the complete markup text, including delimiters.
Namespace declarations are ordinary attributes in this tree, so the example sets ``xmlns:musik`` on the root.

.. erbsland-demo::
    :source: data/DataFormats/XmlExamples.cpp
    :function-blocks: buildXmlMarkup
    :function-blocks-sha256: added138628d3ac825aada7b6ae8a70b88bc4636b3b74619dce1c144ae4c9cc7
    :exec: data/data_formats --demo BuildXmlMarkup
    :source-sha256: a145738dffc0ee7977bd43f71e5b85f85645263c0897e7e4c042b1dd1b60911b

.. code-block:: cpp

    void buildXmlMarkup() {
        // Place document-level markup before the root, then append mixed nodes to it.
        auto document = XmlDocument{};
        document.add(XmlNode::createDeclaration("<?xml version=\"1.0\"?>"_el));
        document.add(XmlNode::createDocType("<!DOCTYPE musik:orkester>"_el));
        auto root = document.addRoot("musik:orkester"_el);
        root->setAttribute("xmlns:musik"_el, "urn:musik"_el);
        root->add(XmlNode::createComment(" første sats "_el));
        root->add(XmlNode::createProcessingInstruction("spil"_el, " langsomt"_el));
        root->add(XmlNode::createCData("violin & bratsch"_el));
        const auto xml = document.toString();
        el::io::printLine("DOCTYPE written: "_el, xml.contains("<!DOCTYPE musik:orkester>"_el));
        el::io::printLine("CDATA written: "_el, xml.contains("<![CDATA[violin & bratsch]]>"_el));
        el::io::printLine("Parsed root: "_el, XmlDocument::fromStringOrThrow(xml).root()->name());
    }

.. erbsland-ansi::
    :escape-char: ␛

    DOCTYPE written: true
    CDATA written: true
    Parsed root: musik:orkester

.. erbsland-demo-end::

Serializing XML Text
====================

:cpp:func:`XmlDocument::toString <erbsland::data::xml::XmlDocument::toString>` writes the complete document as a
``String``.
Ordinary text and attributes are escaped as needed, so an ampersand in a value becomes ``&amp;`` on output.
An element without children is written with ``/>``.
The writer preserves the order of top-level nodes, child nodes, and attributes.
It does not add indentation or change the content merely to make the output look pretty: whitespace in mixed content can
be meaningful.

The example builds a document whose text contains ``&``, writes it, and reads it back.
The parsed text is the original value, even though the serialized spelling is different.

.. erbsland-demo::
    :source: data/DataFormats/XmlExamples.cpp
    :function-blocks: serializeXmlDocument
    :function-blocks-sha256: 99b4e68cdf6bdb09566c924ba38b8ff8117a4118b3508a520be6856cf2d35c07
    :exec: data/data_formats --demo SerializeXmlDocument
    :source-sha256: a145738dffc0ee7977bd43f71e5b85f85645263c0897e7e4c042b1dd1b60911b

.. code-block:: cpp

    void serializeXmlDocument() {
        // The writer escapes ordinary text, and the parser restores its value.
        auto document = XmlDocument{};
        auto root = document.addRoot("instrument"_el, "violin & bratsch"_el);
        root->setAttribute("gruppe"_el, "strygere"_el);
        const auto xml = document.toString();
        el::io::printLine("XML: "_el, xml);
        el::io::printLine("Text: "_el, XmlDocument::fromStringOrThrow(xml).root()->textContentOrThrow());
    }

.. erbsland-ansi::
    :escape-char: ␛

    XML: <instrument gruppe="strygere">violin &amp; bratsch</instrument>
    Text: violin & bratsch

.. erbsland-demo-end::

The writer requires one root element.
It also checks the placement of declarations and DOCTYPE nodes, XML names, characters, comments, CDATA, and processing
instructions; invalid constructed content raises :cpp:class:`ParameterError <erbsland::err::ParameterError>`.
A parsed document can be written again, but this is a *structural* round trip rather than a promise of byte-for-byte
output.
For example, the writer uses double quotes for attributes, escapes text in its own way, and writes empty elements with
``/>``.
:cpp:class:`XmlFormatOptions <erbsland::data::xml::XmlFormatOptions>` is currently an empty settings type, so there
are no serialization switches to configure.

Understanding the Node Types
============================

:cpp:enum:`XmlNodeType <erbsland::data::xml::XmlNodeType>` tells you what each node represents.
The table shows what is retained from parsed XML and how to create the corresponding node yourself.

.. list-table:: XML nodes in Erbsland Core
    :header-rows: 1
    :widths: 22 36 42

    * - Node type
      - Content and access
      - Construction
    * - ``Element``
      - Qualified ``name()``, ordered ``attributes()`` and ``children()``.
      - ``addRoot()``, ``addElement()``, or ``createElement()``.
    * - ``Text``
      - Character data in ``text()``.
      - ``addText()`` or ``createText()``.
    * - ``CData``
      - CDATA content in ``text()`` without its delimiters.
      - ``createCData()``.
    * - ``Comment``
      - Comment content in ``text()`` without its delimiters.
      - ``createComment()``.
    * - ``ProcessingInstruction``
      - Target in ``name()`` and following content in ``text()``.
      - ``createProcessingInstruction()``.
    * - ``Declaration``
      - Complete XML declaration markup in ``text()``.
      - ``createDeclaration()``.
    * - ``DocType``
      - Complete DOCTYPE markup in ``text()``.
      - ``createDocType()``.
    * - ``EntityReference``
      - Reference name in ``name()``, without ``&`` and ``;``.
      - ``createEntityReference()``.

The :cpp:class:`XmlAttribute <erbsland::data::xml::XmlAttribute>` objects in ``attributes()`` also retain their order
and qualified names.
A parsed attribute's ``value()`` can retain reference spellings from the source; call ``getAttribute()`` on its element
when you want decoded text.
A value set with ``setAttribute()`` is plain text and will be escaped by the writer.

Navigating a Parsed Tree
========================

:cpp:func:`XmlDocument::root <erbsland::data::xml::XmlDocument::root>` returns the root element, or an empty pointer
for an empty document you are still building.
``nodes()`` gives the ordered top-level nodes, including any declaration, DOCTYPE, comments, and processing
instructions.
For an element, :cpp:func:`children <erbsland::data::xml::XmlNode::children>` exposes its ordered children.
Look at each child's ``type()`` before treating it as an element: text, comments, and other nodes can appear between
elements.

:cpp:func:`getAttribute <erbsland::data::xml::XmlNode::getAttribute>` returns optional decoded text.
It is empty when the attribute is absent or contains an unresolved reference.
:cpp:func:`textContent <erbsland::data::xml::XmlNode::textContent>` gathers descendant text and CDATA in order,
decoding references it understands and ignoring comments and processing instructions.
If the complete text cannot be known because a reference is unresolved, it returns no value.
Use :cpp:func:`textContentOrThrow <erbsland::data::xml::XmlNode::textContentOrThrow>` when that condition should be an
error; it raises :cpp:class:`LogicError <erbsland::err::LogicError>`.

This mixed-content example shows why indexing children as if they were all elements would be misleading.
The first child is text, followed by an element, then more text.

.. erbsland-demo::
    :source: data/DataFormats/XmlExamples.cpp
    :function-blocks: inspectXmlDocument
    :function-blocks-sha256: b3833eed555529334ec7dc540ba3a62da7859a47c1c9271516f6be6a52b268ac
    :exec: data/data_formats --demo InspectXmlDocument
    :source-sha256: a145738dffc0ee7977bd43f71e5b85f85645263c0897e7e4c042b1dd1b60911b

.. code-block:: cpp

    void inspectXmlDocument() {
        // Mixed content retains text and elements as separate ordered children.
        const auto source =
            "<gruppe navn=\"træblæsere\">Solo: <instrument>fløjte</instrument> og <instrument>obo</instrument></gruppe>"_el;
        const auto document = XmlDocument::fromStringOrThrow(source);
        const auto root = document.root();
        el::io::printLine("Group: "_el, root->getAttribute("navn"_el).value());
        el::io::printLine("First child is text: "_el, root->children().get(el::ItemIndex{0U})->type() == XmlNodeType::Text);
        el::io::printLine("First instrument: "_el, root->children().get(el::ItemIndex{1U})->textContentOrThrow());
        el::io::printLine("All text: "_el, root->textContentOrThrow());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Group: træblæsere
    First child is text: true
    First instrument: fløjte
    All text: Solo: fløjte og obo

.. erbsland-demo-end::

References and Preserved Markup
===============================

XML has five predefined named references, such as ``&amp;`` and ``&lt;``, and numeric character references such as
``&#65;``.
Core can decode those when you ask for an attribute or text content.
Other named references remain distinct ``EntityReference`` nodes in element content.
The parser preserves a DOCTYPE declaration, including its internal subset, but does not expand DTD entities or fetch
external resources.
As a result, a document can parse successfully while ``textContent()`` returns no value for a subtree with an unresolved
reference.
Do not assume that the declaration of an entity in the DOCTYPE makes its value available to the DOM.

The example keeps declaration, DOCTYPE, CDATA, comment, and entity reference nodes in order.
It can read the CDATA alone, while the group's attribute and complete text are unavailable because ``&leder;`` is
unresolved.

.. erbsland-demo::
    :source: data/DataFormats/XmlExamples.cpp
    :function-blocks: inspectXmlMarkup
    :function-blocks-sha256: b8c2398431dd2890b34d295414c5f709303e83d9bdb4c86d0f54ecadfa6fb322
    :exec: data/data_formats --demo InspectXmlMarkup
    :source-sha256: a145738dffc0ee7977bd43f71e5b85f85645263c0897e7e4c042b1dd1b60911b

.. code-block:: cpp

    void inspectXmlMarkup() {
        // The DTD entity is retained as a reference, including inside the attribute.
        const auto source =
            "<?xml version=\"1.0\"?><!DOCTYPE orkester [<!ENTITY leder \"Maja\">]>"
            "<orkester><gruppe leder=\"&leder;\"><![CDATA[fløjte & obo]]><!-- solo -->&leder;</gruppe></orkester>"_el;
        const auto document = XmlDocument::fromStringOrThrow(source);
        const auto group = document.root()->children().get(el::ItemIndex{0U});
        el::io::printLine("Top-level nodes: "_el, document.nodes().count().toSizeT());
        el::io::printLine(
            "First child is CDATA: "_el, group->children().get(el::ItemIndex{0U})->type() == XmlNodeType::CData);
        el::io::printLine("Known text: "_el, group->children().get(el::ItemIndex{0U})->textContentOrThrow());
        el::io::printLine("Attribute available: "_el, group->getAttribute("leder"_el).has_value());
        el::io::printLine("All text available: "_el, group->textContent().has_value());
        try {
            (void)group->textContentOrThrow();
        } catch (const el::err::LogicError &) {
            el::io::printLine("Unresolved entity reference"_el);
        }
        el::io::printLine("DOCTYPE retained: "_el, document.toString().contains("<!DOCTYPE"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Top-level nodes: 3
    First child is CDATA: true
    Known text: fløjte & obo
    Attribute available: false
    All text available: false
    Unresolved entity reference
    DOCTYPE retained: true

.. erbsland-demo-end::

Limiting Untrusted Input
========================

:cpp:class:`XmlParseOptions <erbsland::data::xml::XmlParseOptions>` provides four independent limits.
Pass the options as the second argument to either parser form.
A limit violation is a parse failure: ``fromString()`` returns no value and ``fromStringOrThrow()`` raises
``ParseError``.
The defaults allow ordinary documents, but a protocol with a known schema can often use smaller limits.
The following example tightens each limit against the same document.

.. erbsland-demo::
    :source: data/DataFormats/XmlExamples.cpp
    :function-blocks: limitXmlInput
    :function-blocks-sha256: e3e83ae0321df5bcc111297d230114d7e6d5f15ed555dc0735b6af77e3c2db8c
    :exec: data/data_formats --demo LimitXmlInput
    :source-sha256: a145738dffc0ee7977bd43f71e5b85f85645263c0897e7e4c042b1dd1b60911b

.. code-block:: cpp

    void limitXmlInput() {
        // Each limit rejects the same source for an independent reason.
        const auto source = "<orkester><gruppe navn=\"strygere\"><instrument>violin</instrument></gruppe></orkester>"_el;
        const auto input = XmlParseOptions{}.setMaximumInputLength(el::ByteLength{10U});
        const auto nesting = XmlParseOptions{}.setMaximumNesting(el::ItemCount{2U});
        const auto nodes = XmlParseOptions{}.setMaximumNodeCount(el::ItemCount{2U});
        const auto strings = XmlParseOptions{}.setMaximumStringLength(el::ByteLength{4U});
        el::io::printLine("Input accepted: "_el, XmlDocument::fromString(source, input).has_value());
        el::io::printLine("Nesting accepted: "_el, XmlDocument::fromString(source, nesting).has_value());
        el::io::printLine("Nodes accepted: "_el, XmlDocument::fromString(source, nodes).has_value());
        el::io::printLine("Strings accepted: "_el, XmlDocument::fromString(source, strings).has_value());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Input accepted: false
    Nesting accepted: false
    Nodes accepted: false
    Strings accepted: false

.. erbsland-demo-end::

Maximum Input Length
--------------------

``setMaximumInputLength(ByteLength{...})`` limits the byte length of the complete source ``String`` before parsing.
The default is 16 MiB.
The demo's ten-byte budget rejects the whole document, regardless of the size of any individual value.

Maximum Nesting
---------------

``setMaximumNesting(ItemCount{...})`` limits element depth; the default is 64. The root counts as the first level.
With a limit of two, the demo can enter ``orkester`` and ``gruppe`` but rejects their nested ``instrument`` element.

Maximum Node Count
------------------

``setMaximumNodeCount(ItemCount{...})`` limits all parsed nodes, including elements, text, comments, declarations,
DOCTYPE nodes, processing instructions, and entity references.
The default is 1,000,000. Attributes are held on their element rather than counted as separate nodes.
The demo's budget of two nodes is exhausted before parsing the nested instrument.

Maximum String Length
---------------------

``setMaximumStringLength(ByteLength{...})`` limits each parsed name, text segment, attribute value, and other node
content separately.
The default is 8 MiB.
This is a per-value limit, while ``maximumInputLength`` still bounds the complete document.
The demo's four-byte limit rejects the root name ``orkester``.
