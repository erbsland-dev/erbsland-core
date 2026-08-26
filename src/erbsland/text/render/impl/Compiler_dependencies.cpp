// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Compiler.hpp"

#include "CompiledBlock.hpp"
#include "CompiledExtends.hpp"
#include "CompiledInclude.hpp"
#include "CompiledLayout.hpp"
#include "ExpressionCompiler.hpp"
#include "ProgramError.hpp"

#include "../RenderError.hpp"

#include "../../../text/Literals.hpp"
#include "../../StringFormat.hpp"

#include <algorithm>

namespace erbsland::text::render::impl {

using namespace text::literals;

void Compiler::compileInclude(Tokenizer &tokenizer, const unit::CodeLocation statementLocation) {
    if (tokenizer.current().kind != TokenKind::String) {
        throwSyntax("An 'include' statement requires one static string layout name."_el, tokenizer.current().location);
    }
    const auto name = tokenizer.current().text;
    tokenizer.advance();

    auto ignoreMissing = false;
    auto contextModeSeen = false;
    auto withContext = true;
    while (tokenizer.current().kind != TokenKind::TagEnd) {
        if (!tokenizer.current().isIdentifier()) {
            throwSyntax("An 'include' statement contains an unsupported modifier."_el, tokenizer.current().location);
        }
        const auto modifier = tokenizer.current().text;
        const auto modifierLocation = tokenizer.current().location;
        tokenizer.advance();
        if (modifier == "ignore"_el) {
            if (ignoreMissing) {
                throwSyntax("An 'include' statement must not repeat 'ignore missing'."_el, modifierLocation);
            }
            if (!tokenizer.current().isIdentifier() || tokenizer.current().text != "missing"_el) {
                throwSyntax(
                    "The 'ignore' include modifier must be followed by 'missing'."_el, tokenizer.current().location);
            }
            ignoreMissing = true;
            tokenizer.advance();
            continue;
        }
        if (modifier == "with"_el || modifier == "without"_el) {
            if (contextModeSeen) {
                throwSyntax("An 'include' statement must specify its context mode at most once."_el, modifierLocation);
            }
            if (!tokenizer.current().isIdentifier() || tokenizer.current().text != "context"_el) {
                throwSyntax("The include context modifier must end with 'context'."_el, tokenizer.current().location);
            }
            contextModeSeen = true;
            withContext = modifier == "with"_el;
            tokenizer.advance();
            continue;
        }
        throwSyntax("An 'include' statement contains an unknown modifier."_el, modifierLocation);
    }
    if (!_dependencyResolver) {
        throwSyntax("Static includes require a configured dependency resolver."_el, statementLocation);
    }

    auto dependency = ConstCompiledLayoutPtr{};
    try {
        dependency = _dependencyResolver(name, ignoreMissing);
    } catch (const RenderError &error) {
        auto context = error.context();
        context.addOuterFrame(StringFormat{"{}:{}"_el}.build(_layout, statementLocation.toString()));
        throw RenderError{std::move(context), error.cause()};
    }
    if (dependency == nullptr && !ignoreMissing) {
        throwSyntax("A required static include resolved to no layout generation."_el, statementLocation);
    }
    const auto includeIndex = _includes.size();
    _includes.emplace_back(
        std::make_shared<const CompiledInclude>(
            name, std::move(dependency), withContext, ignoreMissing, statementLocation));
    writer().writeInclude(includeIndex, statementLocation);
}

void Compiler::compileExtends(Tokenizer &tokenizer, const unit::CodeLocation statementLocation) {
    if (!isLayoutTopLevel()) {
        throwSyntax("An 'extends' statement is allowed only at layout level."_el, statementLocation);
    }
    if (_extends != nullptr) {
        throwSyntax("A layout must not contain multiple 'extends' statements."_el, statementLocation);
    }
    if (tokenizer.current().kind != TokenKind::String) {
        throwSyntax("An 'extends' statement requires one static string layout name."_el, tokenizer.current().location);
    }
    const auto name = tokenizer.current().text;
    tokenizer.advance();
    if (tokenizer.current().kind != TokenKind::TagEnd) {
        throwSyntax("An 'extends' statement does not accept modifiers."_el, tokenizer.current().location);
    }
    if (_invalidExtendingContent.has_value()) {
        throwSyntax(
            "An extending layout may contain only setup assignments, blocks, comments, and whitespace outside blocks."_el,
            *_invalidExtendingContent);
    }
    if (!_dependencyResolver) {
        throwSyntax("Static inheritance requires a configured dependency resolver."_el, statementLocation);
    }

    auto parent = ConstCompiledLayoutPtr{};
    try {
        parent = _dependencyResolver(name, false);
    } catch (const RenderError &error) {
        auto context = error.context();
        context.addOuterFrame(StringFormat{"{}:{}"_el}.build(_layout, statementLocation.toString()));
        throw RenderError{std::move(context), error.cause()};
    }
    if (parent == nullptr) {
        throwSyntax("A required static parent resolved to no layout generation."_el, statementLocation);
    }
    _extends = std::make_shared<const CompiledExtends>(name, std::move(parent), statementLocation);
}

void Compiler::compileBlock(Tokenizer &tokenizer, const unit::CodeLocation statementLocation) {
    if (tokenizer.current().kind != TokenKind::Identifier) {
        throwSyntax("A 'block' statement requires one ASCII identifier name."_el, tokenizer.current().location);
    }
    const auto name = tokenizer.current().text;
    tokenizer.advance();
    if (tokenizer.current().kind != TokenKind::TagEnd) {
        throwSyntax("A 'block' statement accepts only its name."_el, tokenizer.current().location);
    }
    if (_blockNames.contains(name)) {
        throwSyntax("A layout must not declare the same block name more than once."_el, statementLocation);
    }
    _blockNames.set(name, true);
    writer().writeRenderBlock(addConstant(name), statementLocation);
    _programs.emplace_back(ProgramContext{name, statementLocation, {}, {}});
}

void Compiler::compileEndBlock(Tokenizer &tokenizer, const unit::CodeLocation statementLocation) {
    if (tokenizer.current().kind != TokenKind::TagEnd) {
        throwSyntax("An 'endblock' statement does not accept arguments."_el, tokenizer.current().location);
    }
    if (isRootProgram()) {
        throwSyntax("An 'endblock' statement requires an open 'block'."_el, statementLocation);
    }
    if (!controls().empty()) {
        throwSyntax("An 'endblock' statement cannot cross an open control statement."_el, statementLocation);
    }
    writer().writeEnd(statementLocation);
    auto context = std::move(_programs.back());
    _programs.pop_back();
    _compiledBlocks.set(
        context.name, std::make_shared<const CompiledBlock>(context.writer.takeProgram(), context.location));
}

}
