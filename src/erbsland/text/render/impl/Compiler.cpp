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

Compiler::Compiler(
    String layout,
    LayoutSource source,
    EnvironmentOptions options,
    StringMap<FilterFn> filters,
    DependencyResolverFn dependencyResolver) :
    _layout{std::move(layout)},
    _source{std::move(source)},
    _options{std::move(options)},
    _applicationFilters{std::move(filters)},
    _dependencyResolver{std::move(dependencyResolver)} {
    _programs.emplace_back();
}

auto Compiler::compile() -> ConstCompiledLayoutPtr {
    if (!_source.text().isValidUtf8()) {
        throwSyntax("The layout source is not valid UTF-8."_el, {});
    }

    auto tokenizer = Tokenizer{_layout, _source.origin(), _source.text(), _options};
    tokenizer.advance();
    while (tokenizer.current().kind != TokenKind::End) {
        switch (tokenizer.current().kind) {
        case TokenKind::Text:
            emitText(tokenizer.current().text, tokenizer.current().location);
            tokenizer.advance();
            break;
        case TokenKind::ExpressionBegin:
            if (isLayoutTopLevel()) {
                noteExtendingContent(tokenizer.current().location);
            }
            tokenizer.advance();
            compileExpression(tokenizer);
            tokenizer.advance();
            break;
        case TokenKind::StatementBegin:
            tokenizer.advance();
            compileStatement(tokenizer);
            tokenizer.advance();
            break;
        default:
            throwSyntax("The layout tokenizer produced an unexpected token."_el, tokenizer.current().location);
        }
    }
    if (_programs.size() > 1U) {
        throwSyntax("A 'block' statement has no matching 'endblock'."_el, _programs.back().location);
    }
    if (!controls().empty()) {
        throwUnclosedControl(controls().back());
    }

    _setupWriter.writeEnd(tokenizer.current().location);
    writer().writeEnd(tokenizer.current().location);
    auto setupProgram = _setupWriter.takeProgram();
    auto bodyProgram = writer().takeProgram();
    return std::make_shared<CompiledLayout>(
        std::move(_layout),
        std::move(_source),
        std::move(_constants),
        std::move(setupProgram),
        std::move(bodyProgram),
        std::move(_applicationFilters),
        std::move(_compiledBlocks),
        std::move(_includes),
        std::move(_extends));
}

void Compiler::compileExpression(Tokenizer &tokenizer) {
    const auto location = tokenizer.current().location;
    auto compiler = ExpressionCompiler{
        _layout,
        _source.origin(),
        tokenizer,
        _constants,
        writer(),
        _applicationFilters,
        !isRootProgram(),
        true,
        _options.escapeFormatForLayout(_layout)};
    compiler.compile();
    writer().writeEmitValue(compiler.outputFormat(), location);
}

void Compiler::compileValueExpression(Tokenizer &tokenizer) {
    compileValueExpression(tokenizer, writer(), !isRootProgram());
}

void Compiler::compileValueExpression(Tokenizer &tokenizer, ProgramWriter &targetWriter, const bool allowSuper) {
    ExpressionCompiler{_layout, _source.origin(), tokenizer, _constants, targetWriter, _applicationFilters, allowSuper}
        .compile();
}

void Compiler::compileStatement(Tokenizer &tokenizer) {
    const auto statementLocation = tokenizer.current().location;
    if (tokenizer.current().kind == TokenKind::TagEnd) {
        throwSyntax("A layout statement must not be empty."_el, statementLocation);
    }
    if (!tokenizer.current().isIdentifier()) {
        throwSyntax("A layout statement must start with an ASCII keyword."_el, statementLocation);
    }
    const auto keyword = tokenizer.current().text;
    tokenizer.advance();
    const auto argumentsLocation = tokenizer.current().location;

    if (keyword == "extends"_el) {
        compileExtends(tokenizer, statementLocation);
        return;
    }
    if (keyword == "block"_el) {
        compileBlock(tokenizer, statementLocation);
        return;
    }
    if (keyword == "endblock"_el) {
        compileEndBlock(tokenizer, statementLocation);
        return;
    }
    if (keyword == "if"_el) {
        if (isLayoutTopLevel()) {
            noteExtendingContent(statementLocation);
        }
        if (tokenizer.current().kind == TokenKind::TagEnd) {
            throwSyntax("An 'if' statement requires an expression."_el, argumentsLocation);
        }
        compileValueExpression(tokenizer);
        controls().emplace_back(IfBlock{statementLocation, writer().writeJumpIfFalse(statementLocation), {}, false});
        return;
    }
    if (keyword == "elif"_el) {
        if (controls().empty()) {
            throwSyntax("An 'elif' statement requires an open 'if'."_el, statementLocation);
        }
        auto *blockPointer = std::get_if<IfBlock>(&controls().back());
        if (blockPointer == nullptr) {
            throwSyntax("An 'elif' statement cannot cross an open 'for' statement."_el, statementLocation);
        }
        auto &block = *blockPointer;
        if (block.hasElse) {
            throwSyntax("An 'elif' statement must not follow 'else'."_el, statementLocation);
        }
        if (tokenizer.current().kind == TokenKind::TagEnd) {
            throwSyntax("An 'elif' statement requires an expression."_el, argumentsLocation);
        }
        block.endJumps.emplace_back(writer().writeJump(statementLocation));
        writer().patchJump(*block.pendingFalse);
        compileValueExpression(tokenizer);
        block.pendingFalse = writer().writeJumpIfFalse(statementLocation);
        return;
    }
    if (keyword == "else"_el) {
        if (!controls().empty() && tokenizer.current().kind != TokenKind::TagEnd) {
            throwSyntax("An 'else' statement does not accept arguments."_el, argumentsLocation);
        }
        if (controls().empty()) {
            throwSyntax("An 'else' statement requires an open 'if'."_el, statementLocation);
        }
        if (auto *forBlock = std::get_if<ForBlock>(&controls().back())) {
            if (forBlock->hasElse) {
                throwSyntax("A 'for' statement must not contain multiple 'else' branches."_el, statementLocation);
            }
            writer().writeJumpTo(forBlock->iterationLabel, statementLocation);
            writer().patchJump(forBlock->pendingExit);
            forBlock->elseEnd = writer().writeEndIterationIfNotEmpty(statementLocation);
            forBlock->hasElse = true;
            return;
        }
        auto &block = std::get<IfBlock>(controls().back());
        if (block.hasElse) {
            throwSyntax("An 'if' statement must not contain multiple 'else' branches."_el, statementLocation);
        }
        block.endJumps.emplace_back(writer().writeJump(statementLocation));
        writer().patchJump(*block.pendingFalse);
        block.pendingFalse.reset();
        block.hasElse = true;
        return;
    }
    if (keyword == "endif"_el) {
        if (!controls().empty() && tokenizer.current().kind != TokenKind::TagEnd) {
            throwSyntax("An 'endif' statement does not accept arguments."_el, argumentsLocation);
        }
        if (controls().empty()) {
            throwSyntax("An 'endif' statement requires an open 'if'."_el, statementLocation);
        }
        if (!std::holds_alternative<IfBlock>(controls().back())) {
            throwSyntax("An 'endif' statement cannot cross an open 'for' statement."_el, statementLocation);
        }
        auto block = std::get<IfBlock>(std::move(controls().back()));
        controls().pop_back();
        if (block.pendingFalse.has_value()) {
            writer().patchJump(*block.pendingFalse);
        }
        for (const auto jump : block.endJumps) {
            writer().patchJump(jump);
        }
        return;
    }
    if (keyword == "set"_el) {
        compileSet(tokenizer);
        return;
    }
    if (keyword == "for"_el) {
        if (isLayoutTopLevel()) {
            noteExtendingContent(statementLocation);
        }
        compileFor(tokenizer, statementLocation);
        return;
    }
    if (keyword == "endfor"_el) {
        compileEndFor(tokenizer, statementLocation);
        return;
    }
    if (keyword == "include"_el) {
        if (isLayoutTopLevel()) {
            noteExtendingContent(statementLocation);
        }
        compileInclude(tokenizer, statementLocation);
        return;
    }
    throwSyntax("The layout contains an unsupported statement."_el, statementLocation);
}

void Compiler::compileSet(Tokenizer &tokenizer) {
    const auto hoisted = isLayoutTopLevel();
    auto &targetWriter = hoisted ? _setupWriter : writer();
    const auto argumentsLocation = tokenizer.current().location;
    if (tokenizer.current().kind == TokenKind::TagEnd) {
        throwSyntax("A 'set' statement requires a name and expression."_el, argumentsLocation);
    }
    if (!tokenizer.current().isIdentifier()) {
        throwSyntax("A 'set' target must be an ASCII identifier."_el, argumentsLocation);
    }
    const auto name = tokenizer.current().text;
    const auto nameLocation = tokenizer.current().location;
    if (name == "true"_el || name == "false"_el || name == "and"_el || name == "or"_el || name == "not"_el ||
        name == "none"_el || name == "null"_el) {
        throwSyntax("A 'set' target must not be a reserved expression keyword."_el, nameLocation);
    }
    if (name == "loop"_el && isInsideFor()) {
        throwSyntax("The special 'loop' name cannot be assigned inside an iteration."_el, nameLocation);
    }
    tokenizer.advance();
    if (tokenizer.current().kind != TokenKind::Assign) {
        throwSyntax("A 'set' statement requires '=' after its target."_el, tokenizer.current().location);
    }
    tokenizer.advance();
    if (tokenizer.current().kind == TokenKind::TagEnd) {
        throwSyntax("A 'set' statement requires an expression after '='."_el, tokenizer.current().location);
    }
    compileValueExpression(tokenizer, targetWriter, !isRootProgram());
    targetWriter.writeStoreName(addConstant(name), nameLocation);
}

void Compiler::compileFor(Tokenizer &tokenizer, const unit::CodeLocation statementLocation) {
    const auto firstLocation = tokenizer.current().location;
    if (!tokenizer.current().isIdentifier()) {
        throwSyntax("A 'for' statement requires an ASCII target name."_el, firstLocation);
    }
    const auto firstName = tokenizer.current().text;
    validateForTarget(firstName, firstLocation);
    tokenizer.advance();

    auto secondName = String{};
    auto secondLocation = unit::CodeLocation{};
    if (tokenizer.current().kind == TokenKind::Comma) {
        tokenizer.advance();
        secondLocation = tokenizer.current().location;
        if (!tokenizer.current().isIdentifier()) {
            throwSyntax("A map 'for' statement requires a second ASCII target name after ','."_el, secondLocation);
        }
        secondName = tokenizer.current().text;
        validateForTarget(secondName, secondLocation);
        if (firstName == secondName) {
            throwSyntax("The two map iteration targets must be distinct."_el, secondLocation);
        }
        tokenizer.advance();
        if (tokenizer.current().kind == TokenKind::Comma) {
            throwSyntax("A 'for' statement accepts at most two target names."_el, tokenizer.current().location);
        }
    }
    if (tokenizer.current().kind != TokenKind::In) {
        throwSyntax("A 'for' statement requires 'in' after its target names."_el, tokenizer.current().location);
    }
    tokenizer.advance();
    if (tokenizer.current().kind == TokenKind::TagEnd) {
        throwSyntax("A 'for' statement requires an iterable expression after 'in'."_el, tokenizer.current().location);
    }
    compileValueExpression(tokenizer);

    if (secondName.isEmpty()) {
        writer().writeBeginListIteration(statementLocation);
    } else {
        writer().writeBeginMapIteration(statementLocation);
    }
    const auto iterationLabel = writer().markLabel();
    const auto pendingExit = writer().writeNextIteration(statementLocation);
    if (secondName.isEmpty()) {
        writer().writeStoreScopedName(addConstant(firstName), firstLocation);
    } else {
        writer().writeStoreScopedName(addConstant(secondName), secondLocation);
        writer().writeStoreScopedName(addConstant(firstName), firstLocation);
    }
    controls().emplace_back(ForBlock{statementLocation, iterationLabel, pendingExit, {}, false});
}

void Compiler::compileEndFor(Tokenizer &tokenizer, const unit::CodeLocation statementLocation) {
    if (tokenizer.current().kind != TokenKind::TagEnd) {
        throwSyntax("An 'endfor' statement does not accept arguments."_el, tokenizer.current().location);
    }
    if (controls().empty()) {
        throwSyntax("An 'endfor' statement requires an open 'for'."_el, statementLocation);
    }
    if (!std::holds_alternative<ForBlock>(controls().back())) {
        throwSyntax("An 'endfor' statement cannot cross an open 'if' statement."_el, statementLocation);
    }
    auto block = std::get<ForBlock>(std::move(controls().back()));
    controls().pop_back();
    if (block.hasElse) {
        if (!block.elseEnd.has_value()) {
            throw ProgramError{"A loop else branch has no pending end jump."_el};
        }
        writer().patchJump(*block.elseEnd);
    } else {
        writer().writeJumpTo(block.iterationLabel, statementLocation);
        writer().patchJump(block.pendingExit);
        writer().writeEndIteration(statementLocation);
    }
}
auto Compiler::isInsideFor() const noexcept -> bool {
    return std::ranges::any_of(
        controls(), [](const ControlBlock &block) -> bool { return std::holds_alternative<ForBlock>(block); });
}

void Compiler::noteExtendingContent(const unit::CodeLocation location) {
    if (_extends != nullptr) {
        throwSyntax(
            "An extending layout may contain only setup assignments, blocks, comments, and whitespace outside blocks."_el,
            location);
    }
    if (!_invalidExtendingContent.has_value()) {
        _invalidExtendingContent = location;
    }
}

void Compiler::validateForTarget(const String &name, const unit::CodeLocation location) const {
    if (name == "loop"_el) {
        throwSyntax("The special 'loop' name cannot be used as an iteration target."_el, location);
    }
    if (name == "in"_el || name == "if"_el || name == "elif"_el || name == "else"_el || name == "endif"_el ||
        name == "set"_el || name == "for"_el || name == "endfor"_el || name == "extends"_el || name == "block"_el ||
        name == "endblock"_el) {
        throwSyntax("A reserved statement keyword cannot be used as an iteration target."_el, location);
    }
}

void Compiler::emitText(String text, const unit::CodeLocation location) {
    if (text.isEmpty()) {
        return;
    }
    if (isLayoutTopLevel() && !text.trimmed().isEmpty()) {
        noteExtendingContent(location);
    }
    writer().writeEmitText(addConstant(std::move(text)), location);
}

auto Compiler::addConstant(String value) -> std::size_t {
    const auto result = _constants.count().toSizeT();
    _constants.append(std::move(value));
    return result;
}

void Compiler::throwUnclosedControl(const ControlBlock &block) const {
    if (const auto *ifBlock = std::get_if<IfBlock>(&block)) {
        throwSyntax("An 'if' statement has no matching 'endif'."_el, ifBlock->location);
    }
    throwSyntax("A 'for' statement has no matching 'endfor'."_el, std::get<ForBlock>(block).location);
}

void Compiler::throwSyntax(String description, const unit::CodeLocation location) const {
    throw RenderError{
        RenderErrorContext{RenderErrorCategory::Syntax, "Invalid layout syntax"_el, std::move(description)}
            .setLayout(_layout)
            .setOrigin(_source.origin())
            .setLocation(location)};
}

}
