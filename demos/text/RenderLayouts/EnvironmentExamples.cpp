// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/ValueList.hpp>
#include <erbsland/EscapeFormat.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/ItemIndex.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/render/Context.hpp>
#include <erbsland/render/Delimiters.hpp>
#include <erbsland/render/Environment.hpp>
#include <erbsland/render/EnvironmentOptions.hpp>
#include <erbsland/render/FileSystemLoader.hpp>
#include <erbsland/render/RenderError.hpp>
#include <erbsland/render/RenderErrorCategory.hpp>
#include <erbsland/render/RenderErrorContext.hpp>
#include <erbsland/render/RenderLimits.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/TempDirectory.hpp>

namespace demo {

using namespace el::unit;

/// Share stable values across renders through the environment's global context.
///
/// A global context supplies names to every layout rendered by the environment. Replacing it publishes a new
/// snapshot for subsequent calls without changing the layout or the loader.
/// @notest{Demo function verified by the documentation executable.}
void sharedValues() {
    const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    (directory->path() / "current.txt"_el)
        .content()
        .writeTextOrThrow("Corrente: {{ current }}; costa: {{ coast }}\n"_el);

    const auto environment = el::render::Environment::create();
    environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
    environment->setGlobalContext(el::render::Context{}.set("current"_el, "Ligure"_el).set("coast"_el, "Italia"_el));
    el::io::print(environment->render("current.txt"_el));
    environment->setGlobalContext(
        el::render::Context{}.set("current"_el, "Adriatica"_el).set("coast"_el, "Croazia"_el));
    el::io::print(environment->render("current.txt"_el));
}

/// Give one render its own values while retaining shared defaults.
///
/// A local name wins when it is also present in the global context. Other global names remain available to
/// the layout, so a caller only supplies the values that vary for one document.
/// @notest{Demo function verified by the documentation executable.}
void localValues() {
    const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    (directory->path() / "observation.txt"_el)
        .content()
        .writeTextOrThrow("{{ station }}: {{ current }} ({{ season }})\n"_el);

    const auto environment = el::render::Environment::create();
    environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
    environment->setGlobalContext(
        el::render::Context{}.set("station"_el, "Punto nord"_el).set("season"_el, "estate"_el));
    const auto local = el::render::Context{}.set("station"_el, "Punto sud"_el).set("current"_el, "Ligure"_el);
    el::io::print(environment->render("observation.txt"_el, local));
}

/// Add one application filter for a value transformation used by a layout.
///
/// A filter receives the piped value at index zero and any positional arguments after it. Validate the
/// arguments in the callback before reading their concrete types. Reject invalid template arguments with
/// RenderError; the renderer supplies the filter's source location.
/// @notest{Demo function verified by the documentation executable.}
void customFilter() {
    const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    (directory->path() / "current.txt"_el).content().writeTextOrThrow("{{ current | label('Corrente: ') }}\n"_el);

    const auto environment = el::render::Environment::create();
    environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
    environment->addFilter("label"_el, [](const el::render::ValueList &values) -> el::render::Value {
        if (values.count() != ItemCount{2U} || !values.getRef(ItemIndex{0U}).isText() ||
            !values.getRef(ItemIndex{1U}).isText()) {
            throw el::render::RenderError{el::render::RenderErrorContext{
                el::render::RenderErrorCategory::Runtime,
                "Invalid label filter arguments"_el,
                "The label filter needs text and one text prefix."_el}};
        }
        return el::StringList{values.getRef(ItemIndex{1U}).asText(), values.getRef(ItemIndex{0U}).asText()}.join();
    });
    el::io::print(environment->render("current.txt"_el, el::render::Context{}.set("current"_el, "Ligure"_el)));
}

/// Choose delimiters that fit the text format containing a layout.
///
/// Expression, statement, and comment delimiters are configured together before creating the environment.
/// @notest{Demo function verified by the documentation executable.}
void customSyntax() {
    const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    (directory->path() / "current.txt"_el)
        .content()
        .writeTextOrThrow("(# note #)<% if active %>[[ current ]]<% endif %>\n"_el);

    auto options = el::render::EnvironmentOptions{};
    options.setExpressionDelimiters(el::render::Delimiters{"[["_el, "]]"_el})
        .setStatementDelimiters(el::render::Delimiters{"<%"_el, "%>"_el})
        .setCommentDelimiters(el::render::Delimiters{"(#"_el, "#)"_el});
    const auto environment = el::render::Environment::create(options);
    environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
    el::io::print(environment->render(
        "current.txt"_el, el::render::Context{}.set("active"_el, true).set("current"_el, "Ligure"_el)));
}

/// Configure escaping for layouts whose suffix identifies the output format.
///
/// Built-in suffix mappings cover common formats. A longer custom suffix can override one mapping; automatic
/// escaping can also be disabled when a trusted plain-text output needs its original characters.
/// @notest{Demo function verified by the documentation executable.}
void escapingOptions() {
    const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    (directory->path() / "note.html"_el).content().writeTextOrThrow("{{ title }}\n"_el);
    (directory->path() / "note.raw.html"_el).content().writeTextOrThrow("{{ title }}\n"_el);

    auto options = el::render::EnvironmentOptions{};
    options.setEscapeFormatForSuffix(".raw.html"_el, el::EscapeFormat::None);
    const auto environment = el::render::Environment::create(options);
    environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
    const auto context = el::render::Context{}.set("title"_el, "Mare & vento"_el);
    el::io::print(environment->render("note.html"_el, context));
    el::io::print(environment->render("note.raw.html"_el, context));

    auto plainOptions = el::render::EnvironmentOptions{};
    plainOptions.setAutomaticEscapingEnabled(false);
    const auto plainEnvironment = el::render::Environment::create(plainOptions);
    plainEnvironment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
    el::io::print(plainEnvironment->render("note.html"_el, context));
}

/// Bound the amount of text a layout may generate.
///
/// Limits are chosen when the environment is created. A limit failure is a RenderError, so callers can use
/// the same error boundary as for loading and syntax failures.
/// @notest{Demo function verified by the documentation executable.}
void renderingLimit() {
    const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    (directory->path() / "current.txt"_el).content().writeTextOrThrow("Corrente Ligure"_el);

    auto limits = el::render::RenderLimits{};
    limits.setGeneratedOutputBytes(8U);
    auto options = el::render::EnvironmentOptions{};
    options.setRenderLimits(limits);
    const auto environment = el::render::Environment::create(options);
    environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
    try {
        el::io::print(environment->render("current.txt"_el));
    } catch (const el::render::RenderError &) {
        el::io::printLine("The output limit was reached."_el);
    }
}

}
