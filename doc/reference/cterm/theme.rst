.. index::
    single: Terminal Theme Interface

************************
Terminal Theme Interface
************************

Interface
=========

.. doxygenenum:: erbsland::cterm::theme::BlockRole
.. doxygenclass:: erbsland::cterm::theme::BlockStringWithMargins
    :members:
.. doxygenclass:: erbsland::cterm::theme::Element
    :members:
.. doxygenenum:: erbsland::cterm::theme::IdentifierType

.. doxygenclass:: erbsland::cterm::theme::Identifier
    :members:
.. doxygenfunction:: erbsland::cterm::theme::layout::stylePaddingAndMargins(const BlockStringView &text, const ThemeAccessor &themeAccessor, Part textPart = Part::None) noexcept -> BlockStringWithMargins

.. doxygenfunction:: erbsland::cterm::theme::layout::stylePaddingAndMargins(const text::StringView &text, const ThemeAccessor &themeAccessor, Part textPart = Part::None) noexcept -> BlockStringWithMargins

.. doxygenfunction:: erbsland::cterm::theme::layout::stylePaddingCropAndMargins(const BlockStringView &text, bgeo::BlockCoordinate displayWidth, const ThemeAccessor &themeAccessor, Part textPart = Part::Text, Part ellipsisPart = Part::Ellipsis, bgeo::Alignment textAlignmentForEllipsis = bgeo::Alignment::Left) noexcept -> BlockStringWithMargins

.. doxygenfunction:: erbsland::cterm::theme::layout::encloseInBrackets(std::span<BlockString> texts, const ThemeAccessor &themeAccessor, Part bracketPart, Part textPart) noexcept -> BlockStringWithMargins

.. doxygenfunction:: erbsland::cterm::theme::layout::encloseInBrackets(std::span<BlockStringView> texts, const ThemeAccessor &themeAccessor, Part bracketPart, Part textPart) noexcept -> BlockStringWithMargins

.. doxygenfunction:: erbsland::cterm::theme::layout::encloseInBrackets(std::span<text::String> texts, const ThemeAccessor &themeAccessor, Part bracketPart, Part textPart) noexcept -> BlockStringWithMargins

.. doxygenfunction:: erbsland::cterm::theme::layout::encloseInBrackets(const text::StringList &texts, const ThemeAccessor &themeAccessor, Part bracketPart, Part textPart) noexcept -> BlockStringWithMargins

.. doxygenfunction:: erbsland::cterm::theme::layout::encloseInBrackets(const BlockStringView &text, const ThemeAccessor &themeAccessor, Part bracketPart, Part textPart) noexcept -> BlockStringWithMargins

.. doxygenfunction:: erbsland::cterm::theme::layout::encloseInBrackets(const text::StringView &text, const ThemeAccessor &themeAccessor, Part bracketPart, Part textPart) noexcept -> BlockStringWithMargins
.. doxygenstruct:: erbsland::cterm::theme::LayoutRectangles
    :members:
.. doxygenclass:: erbsland::cterm::theme::Part
    :members:
.. doxygenclass:: erbsland::cterm::theme::Properties
    :members:
.. doxygenclass:: erbsland::cterm::theme::PropertyEditor
    :members:
.. doxygenclass:: erbsland::cterm::theme::PropertySheet
    :members:
.. doxygenclass:: erbsland::cterm::theme::Selector
    :members:
.. doxygenclass:: erbsland::cterm::theme::State
    :members:
.. doxygenclass:: erbsland::cterm::theme::States
    :members:
.. doxygenclass:: erbsland::cterm::theme::Tag
    :members:
.. doxygenclass:: erbsland::cterm::theme::Tags
    :members:
.. doxygenclass:: erbsland::cterm::theme::Theme
    :members:
.. doxygenclass:: erbsland::cterm::theme::ThemeAccessor
    :members:
.. doxygenclass:: erbsland::cterm::theme::ThemeBuilder
    :members:
.. doxygenclass:: erbsland::cterm::theme::ThemePainter
    :members:
