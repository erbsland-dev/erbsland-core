// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Identifier.hpp"

#include <functional>
#include <limits>
#include <stdexcept>

namespace erbsland::cterm::theme {

/// A fast theme element identifier.
class Element final {
public:
    using Id = Identifier<IdentifierType::Element>; ///< The underlying element identifier.

public:
    /// Create an empty element identifier.
    constexpr Element() noexcept = default;
    /// Create an element from an element identifier.
    /// @param id The element identifier.
    constexpr Element(const Id id) noexcept : _id{id} {}

public: // operators
    /// Compare two element identifiers.
    [[nodiscard]] constexpr auto operator==(const Element &other) const noexcept -> bool { return _id == other._id; }
    /// Compare two element identifiers.
    [[nodiscard]] constexpr auto operator!=(const Element &other) const noexcept -> bool { return _id != other._id; }

public:
    /// Get the numeric identifier.
    [[nodiscard]] constexpr auto value() const noexcept -> uint16_t { return _id.value(); }
    /// Test if this identifier is non-empty.
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool { return _id.isValid(); }
    /// Create a custom element identifier.
    /// @param id A stable application-defined positive identifier.
    /// @throws std::invalid_argument If `id` is not positive.
    /// @throws std::out_of_range If `id` exceeds the supported custom identifier range.
    [[nodiscard]] static constexpr auto custom(const int32_t id) -> Element {
        if (id <= 0) {
            throw std::invalid_argument{"Custom theme element identifiers must be positive."};
        }
        if (id > std::numeric_limits<uint16_t>::max() - cCustomOffset) {
            throw std::out_of_range{"Custom theme element identifier is too large."};
        }
        return Element{Id{static_cast<uint16_t>(id + cCustomOffset)}};
    }
    /// Get a stable hash for this element identifier.
    [[nodiscard]] constexpr auto hash() const noexcept -> std::size_t { return _id.hash(); }

public:
    /// No specific element.
    static const Element None;
    /// Root element that provides defaults for every theme.
    static const Element Base;
    /// Generic page root.
    static const Element Page;
    /// Generic layout element.
    static const Element Layout;
    /// Generic surface/control element.
    static const Element Surface;
    /// A generic panel element.
    static const Element Panel;
    /// A text box element.
    static const Element TextBox;
    /// A status line element.
    static const Element StatusLine;
    /// A header line element.
    static const Element HeaderLine;
    /// A footer line element.
    static const Element FooterLine;
    /// An action help element.
    static const Element ActionHelp;
    /// A horizontal scroll bar element.
    static const Element HorizontalScrollBar;
    /// A vertical scroll bar element.
    static const Element VerticalScrollBar;
    /// A scroll corner element.
    static const Element ScrollCorner;
    /// A vertical sections layout element.
    static const Element Sections;
    /// A horizontal button layout element.
    static const Element Buttons;
    /// A focusable action button element.
    static const Element Button;
    /// A generic framed one-child layout element.
    static const Element Frame;
    /// A modal choice page element.
    static const Element Choice;
    /// The help viewer page.
    static const Element HelpViewer;
    /// A one-line static text surface.
    static const Element StaticText;
    /// A label text surface.
    static const Element Label;
    /// A dynamic text surface.
    static const Element DynamicText;

private:
    static constexpr auto cCustomOffset = uint16_t{1000};

private:
    Id _id; ///< The numeric element identifier.
};

inline constexpr Element Element::None{Id{0}};
inline constexpr Element Element::Base{Id{1}};
inline constexpr Element Element::Page{Id{2}};
inline constexpr Element Element::Layout{Id{3}};
inline constexpr Element Element::Surface{Id{4}};
inline constexpr Element Element::Panel{Id{5}};
inline constexpr Element Element::TextBox{Id{6}};
inline constexpr Element Element::StatusLine{Id{7}};
inline constexpr Element Element::HeaderLine{Id{8}};
inline constexpr Element Element::FooterLine{Id{9}};
inline constexpr Element Element::ActionHelp{Id{10}};
inline constexpr Element Element::HorizontalScrollBar{Id{11}};
inline constexpr Element Element::VerticalScrollBar{Id{12}};
inline constexpr Element Element::ScrollCorner{Id{13}};
inline constexpr Element Element::Sections{Id{14}};
inline constexpr Element Element::Buttons{Id{15}};
inline constexpr Element Element::Button{Id{16}};
inline constexpr Element Element::Frame{Id{17}};
inline constexpr Element Element::Choice{Id{18}};
inline constexpr Element Element::HelpViewer{Id{19}};
inline constexpr Element Element::StaticText{Id{20}};
inline constexpr Element Element::Label{Id{21}};
inline constexpr Element Element::DynamicText{Id{22}};

}

template <>
struct std::hash<erbsland::cterm::theme::Element> {
    auto operator()(const erbsland::cterm::theme::Element &element) const noexcept -> std::size_t {
        return element.hash();
    }
};
