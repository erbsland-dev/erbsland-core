// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "InputDispatchProbe.hpp"
#include "ReadableBufferDispatchProbe.hpp"
#include "WritableBufferDispatchProbe.hpp"

#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(Input ReadableBuffer WritableBuffer)
class ApiWrapperDispatchTest final : public el::UnitTest {
public:
    void testInputReadKeyAndWaitForKeyDelegateToTheImplementation() {
        auto input = InputDispatchProbe{};

        REQUIRE_EQUAL(input.readKey(std::chrono::milliseconds{-5}), Key{Key::Escape});
        REQUIRE_EQUAL(input._lastTimeout, std::chrono::milliseconds{0});

        REQUIRE_EQUAL(input.readKey(std::chrono::milliseconds{125}), Key{Key::Escape});
        REQUIRE_EQUAL(input._lastTimeout, std::chrono::milliseconds{125});

        REQUIRE_EQUAL(input.waitForKey(), Key{Key::Enter});
        REQUIRE(input._waitForKeyWasCalled);
    }

    void testReadableBufferToMaskOverloadsDelegateToToMaskImpl() {
        auto buffer = ReadableBufferDispatchProbe{};

        REQUIRE_EQUAL(buffer.toMask(erbsland::text::CharSet{"AB"_el}).size(), (bgeo::BlockSize{2, 1}));
        REQUIRE_EQUAL(buffer._characters, erbsland::text::CharSet{"AB"_el});
        REQUIRE_FALSE(buffer._invert);

        REQUIRE_EQUAL(buffer.toMask({U'X', U'Y'}, true).size(), (bgeo::BlockSize{2, 1}));
        REQUIRE_EQUAL(buffer._characters, erbsland::text::CharSet{"XY"_el});
        REQUIRE(buffer._invert);
    }

    void testWritableBufferSetFromAndFillDelegateToImplMethods() {
        auto buffer = WritableBufferDispatchProbe{};
        auto source = Buffer{bgeo::BlockSize{2, 2}};

        buffer.setFrom(source);
        REQUIRE_EQUAL(buffer._lastCall, WritableBufferDispatchProbe::Call::SetFrom);
        REQUIRE_EQUAL(buffer._lastFillChar, Block::space());

        buffer.clearRecording();
        buffer.fill(bgeo::BlockRectangle{0, 0, 2, 2}, Block{U'.'});
        REQUIRE_EQUAL(buffer._lastCall, WritableBufferDispatchProbe::Call::FillBlock);
        REQUIRE_EQUAL(buffer._lastRect, (bgeo::BlockRectangle{0, 0, 2, 2}));
        REQUIRE_EQUAL(buffer._lastFillChar, Block{U'.'});

        buffer.clearRecording();
        buffer.fill(bgeo::BlockRectangle{0, 0, 3, 3}, Tile9Style::create("ABCDEFGHI"_el), Color{fg::Blue, bg::Black});
        REQUIRE_EQUAL(buffer._lastCall, WritableBufferDispatchProbe::Call::FillTile9);
        REQUIRE(buffer._lastTile9Style != nullptr);
        REQUIRE_EQUAL(buffer._lastFrameColor, Color(fg::Blue, bg::Black));
    }

    void testWritableBufferResizeWithModeUsesTheDefaultImplementation() {
        auto buffer = WritableBufferDispatchProbe{};

        buffer.resize(bgeo::BlockSize{5, 3}, BufferResizeMode::Fast, Block{U'.'});
        REQUIRE_EQUAL(buffer._lastCall, WritableBufferDispatchProbe::Call::Resize);
        REQUIRE_EQUAL(buffer._lastResizeSize, (bgeo::BlockSize{5, 3}));
        REQUIRE_EQUAL(buffer._resizeCallCount, 1);

        buffer.clearRecording();
        buffer.resize(bgeo::BlockSize{6, 4}, BufferResizeMode::PreserveContent, Block{U'.'});
        REQUIRE_EQUAL(buffer._lastCall, WritableBufferDispatchProbe::Call::SetFrom);
        REQUIRE_EQUAL(buffer._lastFillChar, Block{U'.'});
        REQUIRE_EQUAL(buffer._resizeCallCount, 1);
        REQUIRE_EQUAL(buffer._lastResizeSize, (bgeo::BlockSize{6, 4}));
    }

    void testWritableBufferFrameWrappersDelegateToGroupedImplMethods() {
        auto buffer = WritableBufferDispatchProbe{};

        buffer.drawFrame(bgeo::BlockRectangle{0, 0, 2, 2}, Block{U'#'});
        REQUIRE_EQUAL(buffer._lastCall, WritableBufferDispatchProbe::Call::FrameBlock);
        REQUIRE_FALSE(buffer._lastOptionalFillBlock.has_value());
        REQUIRE_EQUAL(buffer._lastFrameBlock, Block{U'#'});

        buffer.clearRecording();
        buffer.drawFilledFrame(bgeo::BlockRectangle{0, 0, 2, 2}, Block{U'#'}, Block{U'.'});
        REQUIRE_EQUAL(buffer._lastCall, WritableBufferDispatchProbe::Call::FrameBlock);
        REQUIRE(buffer._lastOptionalFillBlock.has_value());
        REQUIRE_EQUAL(*buffer._lastOptionalFillBlock, Block{U'.'});

        buffer.clearRecording();
        buffer.drawFrame(bgeo::BlockRectangle{0, 0, 3, 3}, FrameStyle::Light, Color{fg::Green, bg::Black});
        REQUIRE_EQUAL(buffer._lastCall, WritableBufferDispatchProbe::Call::FrameChar16);
        REQUIRE(buffer._lastBlock16Style != nullptr);
        REQUIRE_FALSE(buffer._lastOptionalFillBlock.has_value());
        REQUIRE_EQUAL(buffer._lastFrameColor, Color(fg::Green, bg::Black));

        buffer.clearRecording();
        buffer.drawFilledFrame(bgeo::BlockRectangle{0, 0, 3, 3}, FrameStyle::OuterHalfBlock, Block{U'.'});
        REQUIRE_EQUAL(buffer._lastCall, WritableBufferDispatchProbe::Call::FrameTile9);
        REQUIRE(buffer._lastTile9Style != nullptr);
        REQUIRE(buffer._lastOptionalFillBlock.has_value());
        REQUIRE_EQUAL(*buffer._lastOptionalFillBlock, Block{U'.'});

        buffer.clearRecording();
        buffer.drawFrame(bgeo::BlockRectangle{0, 0, 3, 3}, FrameDrawOptions::defaultOptions(), 7);
        REQUIRE_EQUAL(buffer._lastCall, WritableBufferDispatchProbe::Call::FrameOptions);
        REQUIRE_EQUAL(buffer._lastAnimationCycle, std::size_t{7});

        buffer.clearRecording();
        auto border = FrameBorder{FrameStyle::Light, Color{fg::Green, bg::Black}};
        buffer.drawGridLayout(bgeo::BlockPosition{1, 2}, gridLayout({2, 3}, {1, 2}), border);
        REQUIRE_EQUAL(buffer._lastCall, WritableBufferDispatchProbe::Call::GridLayout);
        REQUIRE_EQUAL(buffer._lastPosition, (bgeo::BlockPosition{1, 2}));
        REQUIRE(buffer._lastGridLayout.has_value());
        REQUIRE_EQUAL(buffer._lastGridLayout->size(border), (bgeo::BlockSize{8, 6}));
        REQUIRE_EQUAL(buffer._lastFrameBorder, border);
    }

    void testWritableBufferTextWrappersDelegateToImplMethods() {
        auto buffer = WritableBufferDispatchProbe{};

        buffer.drawBlockText(BlockText{BlockString{"Hello"_el}, bgeo::BlockRectangle{1, 2, 3, 4}});
        REQUIRE_EQUAL(buffer._lastCall, WritableBufferDispatchProbe::Call::TextObject);
        REQUIRE_EQUAL(buffer._lastRect, (bgeo::BlockRectangle{1, 2, 3, 4}));
        REQUIRE_EQUAL(buffer._lastText.length(), BlockCount{5U});
        REQUIRE_EQUAL(buffer._lastText[BlockIndex{0U}], Block{U'H'});
        REQUIRE_EQUAL(buffer._lastText[BlockIndex{4U}], Block{U'o'});
        REQUIRE_EQUAL(buffer._lastAnimationCycle, std::size_t{0});

        buffer.clearRecording();
        buffer.drawBlockText(
            "Hi"_el, bgeo::BlockRectangle{0, 0, 5, 1}, bgeo::Alignment::Center, Color{fg::Yellow, bg::Blue}, 9);
        REQUIRE_EQUAL(buffer._lastCall, WritableBufferDispatchProbe::Call::TextRect);
        REQUIRE_EQUAL(buffer._lastRect, (bgeo::BlockRectangle{0, 0, 5, 1}));
        REQUIRE_EQUAL(buffer._lastText.length(), BlockCount{2U});
        REQUIRE_EQUAL(buffer._lastText[BlockIndex{0U}], Block{U'H'});
        REQUIRE_EQUAL(buffer._lastText[BlockIndex{1U}], Block{U'i'});
        REQUIRE_EQUAL(buffer._lastAlignment, bgeo::Alignment::Center);
        REQUIRE_EQUAL(buffer._lastAnimationCycle, std::size_t{9});
    }

    void testWritableBufferBitmapWrappersDelegateToImplMethods() {
        auto buffer = WritableBufferDispatchProbe{};
        auto bitmap = Bitmap{bgeo::BlockSize{2, 3}};

        buffer.drawBitmap(bitmap, bgeo::BlockPosition{1, 2});
        REQUIRE_EQUAL(buffer._lastCall, WritableBufferDispatchProbe::Call::BitmapPosition);
        REQUIRE_EQUAL(buffer._lastBitmapSize, (bgeo::BlockSize{2, 3}));
        REQUIRE_EQUAL(buffer._lastPosition, (bgeo::BlockPosition{1, 2}));
        REQUIRE_EQUAL(buffer._lastAnimationCycle, std::size_t{0});

        buffer.clearRecording();
        buffer.drawBitmap(
            bitmap,
            bgeo::BlockRectangle{0, 0, 4, 5},
            bgeo::Alignment::BottomRight,
            BitmapDrawOptions::defaultOptions(),
            3);
        REQUIRE_EQUAL(buffer._lastCall, WritableBufferDispatchProbe::Call::BitmapRect);
        REQUIRE_EQUAL(buffer._lastBitmapSize, (bgeo::BlockSize{2, 3}));
        REQUIRE_EQUAL(buffer._lastRect, (bgeo::BlockRectangle{0, 0, 4, 5}));
        REQUIRE_EQUAL(buffer._lastAlignment, bgeo::Alignment::BottomRight);
        REQUIRE_EQUAL(buffer._lastAnimationCycle, std::size_t{3});
    }
};
