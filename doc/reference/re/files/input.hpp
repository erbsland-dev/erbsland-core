#pragma once

#include <erbsland/Char.hpp>
#include <erbsland/re/Input.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/re/RegEx.hpp>
#include <erbsland/text/u32/U32StringView.hpp>

#include <string>
#include <utility>
#include <vector>

// FIXME!
// This should be moved into a demo - to make sure it compiles and works as expected.

class VectorMatch : public erbsland::re::Match32 {
public:
    using Match32::Match32;

public:
    VectorMatch(
        const el::re::ConstRegExPtr &regEx,
        const el::re::CaptureGroupList &captureGroupList,
        std::u32string &&capturedContent,
        const std::size_t offset) :
        Match32(regEx, captureGroupList), _capturedContent{std::move(capturedContent)}, _offset{offset} {}
    ~VectorMatch() override = default;

protected:
    [[nodiscard]] auto getContentForGroup(const el::re::CaptureGroup &group) const noexcept
        -> el::text::U32StringView override {

        return el::text::U32String{_capturedContent}.slice({group.begin() - _offset, group.size()});
    }

private:
    std::u32string _capturedContent;
    std::size_t _offset;
};

class VectorInput : public erbsland::re::Input32 {
public:
    explicit VectorInput(const std::vector<char32_t> &textVector) : _vectorRef(textVector) {}
    ~VectorInput() override = default;

public:
    [[nodiscard]] auto read() -> el::re::CharAndPosition override {
        if (_position >= _vectorRef.size()) {
            return {el::text::Char::endOfData(), _position};
        }
        const auto position = _position;
        const auto character = decodeCodePoint(_vectorRef[position]);
        _position += 1;
        return {character, position};
    }
    [[nodiscard]] auto peek() -> el::re::CharAndPosition override {
        if (_position >= _vectorRef.size()) {
            return {el::text::Char::endOfData(), _position};
        }
        return {decodeCodePoint(_vectorRef[_position]), _position};
    }
    void skip(const std::size_t characterCount) override {
        _position += characterCount;
        if (_position > _vectorRef.size()) {
            _position = _vectorRef.size();
        }
    }
    [[nodiscard]] auto createMatch(el::re::ConstRegExPtr regEx, el::re::CaptureGroupList captureGroupList)
        -> el::re::Match32Ptr override {

        std::u32string capturedContent;
        const std::size_t offset = captureGroupList.front().begin();
        const std::size_t end = captureGroupList.front().end();
        for (std::size_t i = offset; i < end; ++i) {
            capturedContent.push_back(decodeCodePoint(_vectorRef.at(i)).toRawValue());
        }
        return std::make_shared<VectorMatch>(
            std::move(regEx), std::move(captureGroupList), std::move(capturedContent), offset);
    }

private:
    [[nodiscard]] static auto decodeCodePoint(const char32_t codePoint) noexcept -> el::text::Char {
        const auto character = el::text::Char{codePoint};
        return character.isValidUnicode() ? character : el::text::Char::replacement();
    }

    const std::vector<char32_t> &_vectorRef;
    std::size_t _position = 0;
};
