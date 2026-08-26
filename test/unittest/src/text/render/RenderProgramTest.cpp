// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/render/EnvironmentOptions.hpp>
#include <erbsland/text/render/impl/CompiledLayout.hpp>
#include <erbsland/text/render/impl/Compiler.hpp>
#include <erbsland/text/render/impl/Engine.hpp>
#include <erbsland/text/render/impl/Opcode.hpp>
#include <erbsland/text/render/impl/ProgramError.hpp>
#include <erbsland/text/render/impl/ProgramReader.hpp>
#include <erbsland/text/render/impl/ProgramWriter.hpp>
#include <erbsland/text/render/LayoutSource.hpp>
#include <erbsland/text/StringList.hpp>
#include <erbsland/text/ToString.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <bit>
#include <vector>

using namespace el::text::literals;
using namespace el::text::render;
using namespace el::text::render::impl;

TESTED_TARGETS(
    CompiledLayout Compiler Engine Opcode Program ProgramError ProgramInstruction ProgramReader ProgramWriter)
class RenderProgramTest final : public el::UnitTest {
public:
    void testSourceMap() {
        const auto layout =
            Compiler{"page"_el, LayoutSource{"first\n{{ value }}"_el, "memory:page"_el, "1"_el}, EnvironmentOptions{}}
                .compile();
        const auto location = layout->program().locationAt(el::unit::ByteIndex{2U});
        REQUIRE_EQUAL(location.line(), el::unit::LineIndex{1U});
        REQUIRE_EQUAL(location.column(), el::unit::ColumnIndex{3U});

        auto hint = std::size_t{};
        const auto first = layout->program().locationAt(el::unit::ByteIndex{0U}, hint);
        const auto second = layout->program().locationAt(el::unit::ByteIndex{2U}, hint);
        const auto repeated = layout->program().locationAt(el::unit::ByteIndex{0U}, hint);
        REQUIRE_EQUAL(first.line(), el::unit::LineIndex{0U});
        REQUIRE_EQUAL(second, location);
        REQUIRE_EQUAL(repeated, first);
    }

    void testProgramReaderWriterRoundTrip() {
        const auto firstLocation =
            el::unit::CodeLocation{el::unit::LineIndex{2U}, el::unit::ColumnIndex{3U}, el::unit::CpIndex{4U}};
        const auto secondLocation =
            el::unit::CodeLocation{el::unit::LineIndex{5U}, el::unit::ColumnIndex{6U}, el::unit::CpIndex{7U}};
        auto writer = ProgramWriter{};
        writer.writeEmitText(300U, firstLocation);
        writer.writeLoadName(1U, secondLocation);
        writer.writeGetMember(129U, secondLocation);
        writer.writePushTrue(secondLocation);
        writer.writePushFalse(secondLocation);
        writer.writePushInteger(-42, secondLocation);
        writer.writePushFloat(1.5, secondLocation);
        writer.writePushText(7U, secondLocation);
        writer.writeLogicalNot(secondLocation);
        writer.writeComparison(Opcode::Equal, secondLocation);
        writer.writeApplicationFilter(8U, 2U, secondLocation);
        writer.writeBuiltInFilter(BuiltInFilter::Upper, 1U, secondLocation);
        writer.writeStoreName(9U, secondLocation);
        writer.writeInclude(2U, secondLocation);
        writer.writeRenderBlock(10U, secondLocation);
        writer.writeLoadSuper(2U, secondLocation);
        writer.writeEmitValue(secondLocation);
        writer.writeEnd(secondLocation);

        const auto program = writer.takeProgram();
        auto reader = ProgramReader{program};
        WITH_CONTEXT(requireInstruction(reader, Opcode::EmitText, 300U, firstLocation));
        WITH_CONTEXT(requireInstruction(reader, Opcode::LoadName, 1U, secondLocation));
        WITH_CONTEXT(requireInstruction(reader, Opcode::GetMember, 129U, secondLocation));
        WITH_CONTEXT(requireInstruction(reader, Opcode::PushTrue, 0U, secondLocation));
        WITH_CONTEXT(requireInstruction(reader, Opcode::PushFalse, 0U, secondLocation));
        WITH_CONTEXT(
            requireInstruction(reader, Opcode::PushInteger, std::bit_cast<uint64_t>(int64_t{-42}), secondLocation));
        WITH_CONTEXT(requireInstruction(reader, Opcode::PushFloat, std::bit_cast<uint64_t>(1.5), secondLocation));
        WITH_CONTEXT(requireInstruction(reader, Opcode::PushText, 7U, secondLocation));
        WITH_CONTEXT(requireInstruction(reader, Opcode::LogicalNot, 0U, secondLocation));
        WITH_CONTEXT(requireInstruction(reader, Opcode::Equal, 0U, secondLocation));
        WITH_CONTEXT(requireInstruction(reader, Opcode::ApplyApplicationFilter, 34U, secondLocation));
        WITH_CONTEXT(requireInstruction(reader, Opcode::ApplyBuiltInFilter, 9U, secondLocation));
        WITH_CONTEXT(requireInstruction(reader, Opcode::StoreName, 9U, secondLocation));
        WITH_CONTEXT(requireInstruction(reader, Opcode::Include, 2U, secondLocation));
        WITH_CONTEXT(requireInstruction(reader, Opcode::RenderBlock, 10U, secondLocation));
        WITH_CONTEXT(requireInstruction(reader, Opcode::LoadSuper, 2U, secondLocation));
        WITH_CONTEXT(requireInstruction(reader, Opcode::EmitValue, 0U, secondLocation));
        WITH_CONTEXT(requireInstruction(reader, Opcode::End, 0U, secondLocation));
        REQUIRE(reader.isAtEnd());

        writer.writeEnd(firstLocation);
        const auto reusedProgram = writer.takeProgram();
        auto reusedReader = ProgramReader{reusedProgram};
        WITH_CONTEXT(requireInstruction(reusedReader, Opcode::End, 0U, firstLocation));
        REQUIRE(reusedReader.isAtEnd());
    }

    void testUnknownOpcode() {
        const auto layout = layoutFromBytes({0xffU});
        REQUIRE_THROWS_AS(ProgramError, renderEngine(layout, Context{}, Context{}));
    }

    void testTruncatedOperand() {
        const auto layout = layoutFromBytes({static_cast<uint8_t>(Opcode::EmitText), 0x80U});
        REQUIRE_THROWS_AS(ProgramError, renderEngine(layout, Context{}, Context{}));
    }

    void testMalformedFilterInstructions() {
        const auto invalidCount = layoutFromBytes(
            {static_cast<uint8_t>(Opcode::PushNull),
                static_cast<uint8_t>(Opcode::ApplyBuiltInFilter),
                0x03U,
                static_cast<uint8_t>(Opcode::End)});
        REQUIRE_THROWS_AS(ProgramError, renderEngine(invalidCount, Context{}, Context{}));

        const auto invalidIdentifier = layoutFromBytes(
            {static_cast<uint8_t>(Opcode::PushNull),
                static_cast<uint8_t>(Opcode::ApplyBuiltInFilter),
                static_cast<uint8_t>(static_cast<uint8_t>(BuiltInFilter::Count) << 2U),
                static_cast<uint8_t>(Opcode::End)});
        REQUIRE_THROWS_AS(ProgramError, renderEngine(invalidIdentifier, Context{}, Context{}));

        const auto missingApplication = layoutFromBytes(
            {static_cast<uint8_t>(Opcode::PushNull),
                static_cast<uint8_t>(Opcode::ApplyApplicationFilter),
                0x00U,
                static_cast<uint8_t>(Opcode::End)});
        REQUIRE_THROWS_AS(ProgramError, renderEngine(missingApplication, Context{}, Context{}));
    }

    void testTruncatedFixedOperandAndInvalidJump() {
        const auto truncated = layoutFromBytes({static_cast<uint8_t>(Opcode::PushInteger), 0x01U});
        REQUIRE_THROWS_AS(ProgramError, renderEngine(truncated, Context{}, Context{}));

        const auto invalidJump = layoutFromBytes(
            {static_cast<uint8_t>(Opcode::Jump),
                0x01U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                static_cast<uint8_t>(Opcode::End)});
        REQUIRE_THROWS_AS(ProgramError, renderEngine(invalidJump, Context{}, Context{}));
    }

    void testJumpPatchingAndStackValidation() {
        auto writer = ProgramWriter{};
        writer.writePushFalse({});
        const auto jump = writer.writeJumpIfFalse({});
        writer.writePushText(0U, {});
        writer.patchJump(jump);
        writer.writeEnd({});
        const auto layout = std::make_shared<CompiledLayout>(
            "page"_el,
            LayoutSource{""_el, "memory:page"_el, "1"_el},
            el::text::StringList{"unexpected"_el},
            writer.takeProgram());
        REQUIRE_EQUAL(renderEngine(layout, Context{}, Context{}), ""_el);

        auto unpatched = ProgramWriter{};
        static_cast<void>(unpatched.writeJump({}));
        REQUIRE_THROWS_AS(ProgramError, unpatched.takeProgram());

        auto nonEmpty = ProgramWriter{};
        nonEmpty.writePushTrue({});
        nonEmpty.writeEnd({});
        const auto nonEmptyLayout = std::make_shared<CompiledLayout>(
            "page"_el, LayoutSource{""_el, "memory:page"_el, "1"_el}, el::text::StringList{}, nonEmpty.takeProgram());
        REQUIRE_THROWS_AS(ProgramError, renderEngine(nonEmptyLayout, Context{}, Context{}));
    }

    void testIterationProgramRoundTrip() {
        auto invalidBackEdge = ProgramWriter{};
        const auto currentPosition = invalidBackEdge.markLabel();
        REQUIRE_THROWS_AS(ProgramError, invalidBackEdge.writeJumpTo(currentPosition, {}));

        auto writer = ProgramWriter{};
        writer.writeBeginListIteration({});
        writer.writeBeginMapIteration({});
        const auto label = writer.markLabel();
        const auto exit = writer.writeNextIteration({});
        writer.writeStoreScopedName(3U, {});
        writer.writeJumpTo(label, {});
        writer.patchJump(exit);
        writer.writeEndIteration({});
        writer.writeEnd({});

        const auto program = writer.takeProgram();
        auto reader = ProgramReader{program};
        WITH_CONTEXT(requireInstruction(reader, Opcode::BeginListIteration, 0U, {}));
        WITH_CONTEXT(requireInstruction(reader, Opcode::BeginMapIteration, 0U, {}));
        const auto next = reader.read();
        REQUIRE_EQUAL(next.opcode(), Opcode::NextIteration);
        REQUIRE(program.hasInstructionAt(el::unit::ByteIndex::fromSizeT(next.operand())));
        WITH_CONTEXT(requireInstruction(reader, Opcode::StoreScopedName, 3U, {}));
        const auto back = reader.read();
        REQUIRE_EQUAL(back.opcode(), Opcode::Jump);
        REQUIRE(program.hasInstructionAt(el::unit::ByteIndex::fromSizeT(back.operand())));
        WITH_CONTEXT(requireInstruction(reader, Opcode::EndIteration, 0U, {}));
        WITH_CONTEXT(requireInstruction(reader, Opcode::End, 0U, {}));
        REQUIRE(reader.isAtEnd());
    }

    void testMalformedIterationState() {
        auto missingFrame = ProgramWriter{};
        const auto missingFrameExit = missingFrame.writeNextIteration({});
        missingFrame.patchJump(missingFrameExit);
        missingFrame.writeEnd({});
        const auto missingFrameLayout = std::make_shared<CompiledLayout>(
            "page"_el,
            LayoutSource{""_el, "memory:page"_el, "1"_el},
            el::text::StringList{},
            missingFrame.takeProgram());
        REQUIRE_THROWS_AS(ProgramError, renderEngine(missingFrameLayout, Context{}, Context{}));

        auto missingScope = ProgramWriter{};
        missingScope.writeStoreScopedName(0U, {});
        missingScope.writeEnd({});
        const auto missingScopeLayout = std::make_shared<CompiledLayout>(
            "page"_el,
            LayoutSource{""_el, "memory:page"_el, "1"_el},
            el::text::StringList{"value"_el},
            missingScope.takeProgram());
        REQUIRE_THROWS_AS(ProgramError, renderEngine(missingScopeLayout, Context{}, Context{}));

        auto missingEndFrame = ProgramWriter{};
        missingEndFrame.writeEndIteration({});
        missingEndFrame.writeEnd({});
        const auto missingEndLayout = std::make_shared<CompiledLayout>(
            "page"_el,
            LayoutSource{""_el, "memory:page"_el, "1"_el},
            el::text::StringList{},
            missingEndFrame.takeProgram());
        REQUIRE_THROWS_AS(ProgramError, renderEngine(missingEndLayout, Context{}, Context{}));

        auto activeFrame = ProgramWriter{};
        activeFrame.writeLoadName(0U, {});
        activeFrame.writeBeginListIteration({});
        activeFrame.writeEnd({});
        const auto activeFrameLayout = std::make_shared<CompiledLayout>(
            "page"_el,
            LayoutSource{""_el, "memory:page"_el, "1"_el},
            el::text::StringList{"items"_el},
            activeFrame.takeProgram());
        REQUIRE_THROWS_AS(
            ProgramError, renderEngine(activeFrameLayout, Context{}, Context{}.set("items"_el, ValueList{1})));
    }

    void testMalformedIncludeState() {
        auto writer = ProgramWriter{};
        writer.writeInclude(0U, {});
        writer.writeEnd({});
        const auto layout = std::make_shared<CompiledLayout>(
            "page"_el, LayoutSource{""_el, "memory:page"_el, "1"_el}, el::text::StringList{}, writer.takeProgram());
        REQUIRE_THROWS_AS(ProgramError, renderEngine(layout, Context{}, Context{}));
    }

    void testMalformedInheritanceState() {
        auto missingBlock = ProgramWriter{};
        missingBlock.writeRenderBlock(0U, {});
        missingBlock.writeEnd({});
        const auto missingBlockLayout = std::make_shared<CompiledLayout>(
            "page"_el,
            LayoutSource{""_el, "memory:page"_el, "1"_el},
            el::text::StringList{"body"_el},
            missingBlock.takeProgram());
        REQUIRE_THROWS_AS(ProgramError, renderEngine(missingBlockLayout, Context{}, Context{}));

        auto superOutsideBlock = ProgramWriter{};
        superOutsideBlock.writeLoadSuper(1U, {});
        superOutsideBlock.writeEmitValue({});
        superOutsideBlock.writeEnd({});
        const auto superOutsideBlockLayout = std::make_shared<CompiledLayout>(
            "page"_el,
            LayoutSource{""_el, "memory:page"_el, "1"_el},
            el::text::StringList{},
            superOutsideBlock.takeProgram());
        REQUIRE_THROWS_AS(ProgramError, renderEngine(superOutsideBlockLayout, Context{}, Context{}));

        auto zeroDepthSuper = ProgramWriter{};
        zeroDepthSuper.writeLoadSuper(0U, {});
        zeroDepthSuper.writeEnd({});
        const auto zeroDepthLayout = std::make_shared<CompiledLayout>(
            "page"_el,
            LayoutSource{""_el, "memory:page"_el, "1"_el},
            el::text::StringList{},
            zeroDepthSuper.takeProgram());
        REQUIRE_THROWS_AS(ProgramError, renderEngine(zeroDepthLayout, Context{}, Context{}));
    }

    void testVariableLengthConstantOperand() {
        auto sourceParts = el::text::StringList{};
        for (auto index = std::size_t{0U}; index < 130U; ++index) {
            sourceParts.append(
                el::text::StringList{"{{ value"_el, el::text::toString(static_cast<uint64_t>(index)), " }}"_el}.join());
        }
        const auto layout =
            Compiler{"page"_el, LayoutSource{sourceParts.join(), "memory:page"_el, "1"_el}, EnvironmentOptions{}}
                .compile();

        REQUIRE_EQUAL(layout->constantCount(), el::unit::ItemCount{130U});
        REQUIRE_EQUAL(renderEngine(layout, Context{}, Context{}), ""_el);
    }

    void testStackLimit() {
        auto writer = ProgramWriter{};
        for (auto index = std::size_t{0U}; index < 1025U; ++index) {
            writer.writeLoadName(0U, {});
        }
        writer.writeEnd({});
        const auto layout = std::make_shared<CompiledLayout>(
            "page"_el,
            LayoutSource{""_el, "memory:page"_el, "1"_el},
            el::text::StringList{"value"_el},
            writer.takeProgram());

        try {
            static_cast<void>(renderEngine(layout, Context{}, Context{}));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().category(), RenderErrorCategory::Limit);
        }
    }

private:
    [[nodiscard]] static auto renderEngine(
        const ConstCompiledLayoutPtr &layout, const Context &globalContext, const Context &localContext)
        -> el::text::String {
        return Engine{layout, globalContext, localContext}.render();
    }

    void requireInstruction(
        ProgramReader &reader,
        const Opcode expectedOpcode,
        const uint64_t expectedOperand,
        const el::unit::CodeLocation expectedLocation) {
        const auto instruction = reader.read();
        REQUIRE_EQUAL(instruction.opcode(), expectedOpcode);
        REQUIRE_EQUAL(instruction.operand(), expectedOperand);
        REQUIRE_EQUAL(instruction.location(), expectedLocation);
    }

    [[nodiscard]] static auto layoutFromBytes(std::vector<uint8_t> bytes) -> ConstCompiledLayoutPtr {
        auto sourceMap = std::vector<Program::SourceMapEntry>{};
        sourceMap.emplace_back(el::unit::ByteIndex::zero(), el::unit::CodeLocation{});
        return std::make_shared<CompiledLayout>(
            "page"_el,
            LayoutSource{""_el, "memory:page"_el, "1"_el},
            el::text::StringList{},
            Program{el::mem::ByteBlock::fromVector(std::move(bytes)), std::move(sourceMap)});
    }
};
