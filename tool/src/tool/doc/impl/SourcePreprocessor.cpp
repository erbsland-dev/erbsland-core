// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SourcePreprocessor.hpp"

#include "../../cleanup/CleanupText.hpp"

#include <erbsland/cpp/CppTokenizer.hpp>
#include <erbsland/List.hpp>
#include <erbsland/PathError.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/PathRemoveOptions.hpp>
#include <erbsland/PathWalker.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/TempDirectory.hpp>

namespace tool::doc::impl {

using namespace el::text::literals;

SourcePreprocessor::SourcePreprocessor(const frame::ProjectFiles &files) : _files{files} {
}

auto SourcePreprocessor::transform(const el::String &source) -> el::String {
    static const auto reMacroDefinition = el::RegEx::compile(R"re(^\s*#\s*define\s+ERBSLAND_CORE_[A-Z_]+)re"_el);
    static const auto reTrailingReturn = el::RegEx::compile(R"re((?x)
        auto \s+ (?<declarator>.*?)                 # Function name and parameters
        \s+ -> \s* (?<return_type>.*?)              # Trailing return type
        \s* (?<suffix>
            (?:(?:override|final)\s+)*              # Spaced virtual qualifiers
            (?:override|final)? \s*                 # Optional final qualifier
            (?: [;\{]                               # Declaration terminator or function body
              | = \s* (?:0|default|delete) \s* ; )  # Special function definition
        )
    )re"_el);
    static const auto reSpecializedStruct = el::RegEx::compile(R"re((?x)
        (?s) template \s* <> \s* \n        # Explicit specialization prefix
        struct .* \{ \n                    # Struct declaration and opening body
        .* \n \}; \n                       # Body and closing declaration
    )re"_el);
    static const auto reMacroContinuation = el::RegEx::compile(R"re((?x)
        (?m) ^ \s* \# \s* define \s+       # Preprocessor definition
        ERBSLAND_CORE_[A-Z_]+ [^\n]*       # Core macro name and first line
        (?: \\\n [^\n]* )*                 # Backslash-continued lines
        \n                                 # Final line ending
    )re"_el);
    static const auto reMacroInvocation = el::RegEx::compile(R"re((?x)
        (?m) ^ (?<prefix>\s*(?:///\s*)?)   # Retained indentation and optional documentation prefix
        \s* ERBSLAND_CORE_[A-Z_]+          # Core annotation macro
        [^\n]* \n                          # Remaining invocation and line ending
    )re"_el);
    static const auto reInlineSpecialization = el::RegEx::compile(R"re((?x)
        (?s) template<> \s+                # Explicit specialization
        \[\[nodiscard\]\] \s+ inline       # Required attribute and qualifier
        .*? \} \s* \n                      # Definition through its closing brace
    )re"_el);

    auto filtered = el::StringEditor{};
    auto offset = el::ByteIndex{0};
    for (const auto &token : el::cpp::CppTokenizer::tokens(source)) {
        filtered.append(source.slice(el::ByteRange{offset, offset.absoluteDistanceTo(token.position())}));
        if (token.kind() == el::cpp::CppToken::Kind::Directive && reMacroDefinition->match(token.text())) {
            filtered.append("// macro removed\n"_el);
        } else {
            filtered.append(token.text());
        }
        offset = token.range().endIndex();
    }
    filtered.append(source.slice(el::ByteRange::fromBegin(offset)));
    auto text = reTrailingReturn->replaceAll(el::String{filtered}, "{return_type} {declarator} {suffix}"_el);
    text = reSpecializedStruct->replaceAll(text, ""_el);
    text = reMacroContinuation->replaceAll(text, "// macro removed\n"_el);
    text = reMacroInvocation->replaceAll(text, "{prefix}// macro removed\n"_el);
    return reInlineSpecialization->replaceAll(text, ""_el);
}

auto SourcePreprocessor::run(const bool force) -> bool {
    const auto output = _files.checked(el::Path{"_doxygen_input"_el});
    const auto marker = output / ".preprocess-sources-run"_el;
    auto inventory = el::StringEditor{"preprocess-sources-v7\n"_el};
    auto inputs = el::List<frame::ProjectTextFile>{};
    const auto source = _files.checked(el::Path{"src"_el});
    source.walker().walkOrThrow(
        [&](const el::Path &path) -> el::PathWalkStatus {
            auto input = _files.readWalkedTextFile(path);
            inventory.append(path.toRelative(source).toString());
            inventory.append(U'\n');
            inventory.append(cleanup::text::digest(input.text()));
            inventory.append(el::String::fromByteBlock(path.info().lastModified().toByteBlock()));
            inventory.append(U'\n');
            inputs.append(std::move(input));
            return el::PathWalkStatus::Continue;
        },
        el::PathWalkOptions{}
            .setTypes(el::PathType::RegularFile)
            .setSymlinkMode(el::SymlinkMode::Skip)
            .setFilter([](const el::Path &path, el::PathType) -> bool {
                return path.suffix() == ".hpp"_el && path.name() != "all.hpp"_el && !path.name().startsWith("all_"_el);
            }));
    const auto fingerprint = cleanup::text::digest(inventory);
    if (!force && marker.info().isRegularFile() && _files.readText(marker) == fingerprint) {
        return false;
    }
    auto temporary = _files.projectRoot().operations().createTempDirectoryOrThrow();
    for (const auto &input : inputs) {
        const auto destination = temporary->path() / input.path().toRelative(source);
        _files.createDirectories(destination.parent());
        _files.updateText(destination, transform(input.text()));
    }
    _files.updateText(temporary->path() / ".preprocess-sources-run"_el, fingerprint);
    // Never publish a stamp until every transformed header has been written successfully.
    auto backup = _files.projectRoot().operations().createTempDirectoryOrThrow();
    const auto previous = backup->path() / "previous"_el;
    const auto hadOutput = output.info().isDirectory();
    if (hadOutput) {
        output.operations().moveToOrThrow(previous);
    }
    try {
        temporary->path().operations().moveToOrThrow(output);
    } catch (const el::PathError &) {
        if (hadOutput) {
            previous.operations().moveToOrThrow(output);
        }
        throw;
    }
    temporary->setRemoveOnDestroy(false);
    return true;
}

}
