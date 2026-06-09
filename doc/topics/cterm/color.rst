..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*****
Color
*****

The color classes provide access to the standard 16 terminal colors and their combinations.
You use them throughout the library to style terminal output, buffer cells, and text elements.

The following table shows the available foreground and background colors, as well as all combinations of them:

.. erbsland-ansi::
    :escape-char: ␛

                  ␛[97mForegrounds␛[39m                             ␛[97mBackgrounds               |
    ␛[30;100m black              ␛[90;40m bright_black       ␛[97m black              ␛[100m bright_black       |
    ␛[31;40m red                ␛[91m bright_red         ␛[97;41m red                ␛[101m bright_red         |
    ␛[32;40m green              ␛[92m bright_green       ␛[97;42m green              ␛[102m bright_green       |
    ␛[33;40m yellow             ␛[93m bright_yellow      ␛[97;43m yellow             ␛[103m bright_yellow      |
    ␛[34;40m blue               ␛[94m bright_blue        ␛[97;44m blue               ␛[104m bright_blue        |
    ␛[35;40m magenta            ␛[95m bright_magenta     ␛[97;45m magenta            ␛[105m bright_magenta     |
    ␛[36;40m cyan               ␛[96m bright_cyan        ␛[97;46m cyan               ␛[106m bright_cyan        |
    ␛[37;40m white              ␛[97m bright_white       ␛[47m white              ␛[90;107m bright_white       |


    ␛[30;40m  0  ␛[31m  1  ␛[32m  2  ␛[33m  3  ␛[34m  4  ␛[35m  5  ␛[36m  6  ␛[37m  7  ␛[90m  8  ␛[91m  9  ␛[92m 10  ␛[93m 11  ␛[94m 12  ␛[95m 13  ␛[96m 14  ␛[97m 15  |
    ␛[30;41m 16  ␛[31m 17  ␛[32m 18  ␛[33m 19  ␛[34m 20  ␛[35m 21  ␛[36m 22  ␛[37m 23  ␛[90m 24  ␛[91m 25  ␛[92m 26  ␛[93m 27  ␛[94m 28  ␛[95m 29  ␛[96m 30  ␛[97m 31  |
    ␛[30;42m 32  ␛[31m 33  ␛[32m 34  ␛[33m 35  ␛[34m 36  ␛[35m 37  ␛[36m 38  ␛[37m 39  ␛[90m 40  ␛[91m 41  ␛[92m 42  ␛[93m 43  ␛[94m 44  ␛[95m 45  ␛[96m 46  ␛[97m 47  |
    ␛[30;43m 48  ␛[31m 49  ␛[32m 50  ␛[33m 51  ␛[34m 52  ␛[35m 53  ␛[36m 54  ␛[37m 55  ␛[90m 56  ␛[91m 57  ␛[92m 58  ␛[93m 59  ␛[94m 60  ␛[95m 61  ␛[96m 62  ␛[97m 63  |
    ␛[30;44m 64  ␛[31m 65  ␛[32m 66  ␛[33m 67  ␛[34m 68  ␛[35m 69  ␛[36m 70  ␛[37m 71  ␛[90m 72  ␛[91m 73  ␛[92m 74  ␛[93m 75  ␛[94m 76  ␛[95m 77  ␛[96m 78  ␛[97m 79  |
    ␛[30;45m 80  ␛[31m 81  ␛[32m 82  ␛[33m 83  ␛[34m 84  ␛[35m 85  ␛[36m 86  ␛[37m 87  ␛[90m 88  ␛[91m 89  ␛[92m 90  ␛[93m 91  ␛[94m 92  ␛[95m 93  ␛[96m 94  ␛[97m 95  |
    ␛[30;46m 96  ␛[31m 97  ␛[32m 98  ␛[33m 99  ␛[34m 100 ␛[35m 101 ␛[36m 102 ␛[37m 103 ␛[90m 104 ␛[91m 105 ␛[92m 106 ␛[93m 107 ␛[94m 108 ␛[95m 109 ␛[96m 110 ␛[97m 111 |
    ␛[30;47m 112 ␛[31m 113 ␛[32m 114 ␛[33m 115 ␛[34m 116 ␛[35m 117 ␛[36m 118 ␛[37m 119 ␛[90m 120 ␛[91m 121 ␛[92m 122 ␛[93m 123 ␛[94m 124 ␛[95m 125 ␛[96m 126 ␛[97m 127 |
    ␛[30;100m 128 ␛[31m 129 ␛[32m 130 ␛[33m 131 ␛[34m 132 ␛[35m 133 ␛[36m 134 ␛[37m 135 ␛[90m 136 ␛[91m 137 ␛[92m 138 ␛[93m 139 ␛[94m 140 ␛[95m 141 ␛[96m 142 ␛[97m 143 |
    ␛[30;101m 144 ␛[31m 145 ␛[32m 146 ␛[33m 147 ␛[34m 148 ␛[35m 149 ␛[36m 150 ␛[37m 151 ␛[90m 152 ␛[91m 153 ␛[92m 154 ␛[93m 155 ␛[94m 156 ␛[95m 157 ␛[96m 158 ␛[97m 159 |
    ␛[30;102m 160 ␛[31m 161 ␛[32m 162 ␛[33m 163 ␛[34m 164 ␛[35m 165 ␛[36m 166 ␛[37m 167 ␛[90m 168 ␛[91m 169 ␛[92m 170 ␛[93m 171 ␛[94m 172 ␛[95m 173 ␛[96m 174 ␛[97m 175 |
    ␛[30;103m 176 ␛[31m 177 ␛[32m 178 ␛[33m 179 ␛[34m 180 ␛[35m 181 ␛[36m 182 ␛[37m 183 ␛[90m 184 ␛[91m 185 ␛[92m 186 ␛[93m 187 ␛[94m 188 ␛[95m 189 ␛[96m 190 ␛[97m 191 |
    ␛[30;104m 192 ␛[31m 193 ␛[32m 194 ␛[33m 195 ␛[34m 196 ␛[35m 197 ␛[36m 198 ␛[37m 199 ␛[90m 200 ␛[91m 201 ␛[92m 202 ␛[93m 203 ␛[94m 204 ␛[95m 205 ␛[96m 206 ␛[97m 207 |
    ␛[30;105m 208 ␛[31m 209 ␛[32m 210 ␛[33m 211 ␛[34m 212 ␛[35m 213 ␛[36m 214 ␛[37m 215 ␛[90m 216 ␛[91m 217 ␛[92m 218 ␛[93m 219 ␛[94m 220 ␛[95m 221 ␛[96m 222 ␛[97m 223 |
    ␛[30;106m 224 ␛[31m 225 ␛[32m 226 ␛[33m 227 ␛[34m 228 ␛[35m 229 ␛[36m 230 ␛[37m 231 ␛[90m 232 ␛[91m 233 ␛[92m 234 ␛[93m 235 ␛[94m 236 ␛[95m 237 ␛[96m 238 ␛[97m 239 |
    ␛[30;107m 240 ␛[31m 241 ␛[32m 242 ␛[33m 243 ␛[34m 244 ␛[35m 245 ␛[36m 246 ␛[37m 247 ␛[90m 248 ␛[91m 249 ␛[92m 250 ␛[93m 251 ␛[94m 252 ␛[95m 253 ␛[96m 254 ␛[97m 255 |

Usage
=====

Applying Colors Directly
------------------------

Use ``Foreground``, ``Background``, and ``Color`` to apply explicit styling to terminal output or buffer cells.

.. code-block:: cpp

    auto accent = Color{fg::BrightCyan, bg::Black};
    auto warning = accent.overlayWith(Color{fg::BrightYellow, bg::Red});

    terminal.printLine(accent, "Normal status");
    terminal.printLine(warning, "Escalated status");

In this example, ``overlayWith()`` combines two colors.
This lets you derive variations from an existing style while keeping unchanged parts intact.

Use ``Default`` when you want to reset a foreground or background to the terminal’s default color.

Use ``Inherited`` when a layer should keep the color from what is already rendered below—for example when composing
buffer cells or applying partial styling.

Serializing Palette Values
--------------------------

For configuration files or theme definitions, colors can also be treated as stable, named palette entries.

``ColorBase::Value`` is the shared enum behind foreground and background colors.
``ColorPart::fromString()`` and ``toString()`` convert between these enum values and human-readable names.

.. code-block:: cpp

    const auto foreground = Foreground::fromString("bright_cyan");
    const auto background = Background::fromString("black");
    const auto accent = Color{foreground, background};

    terminal.printLine(accent, "Configured from text");

This makes it easy to load themes from text files or serialize color choices for later reuse.

Building Animated Palettes
--------------------------

``ColorSequence`` represents an ordered list of colors.
You can use it to build gradients, rotating palettes, or animated visual effects.

.. code-block:: cpp

    auto titleColors = ColorSequence{
        Color{fg::BrightBlue, bg::Black},
        Color{fg::BrightCyan, bg::Black},
        Color{fg::BrightMagenta, bg::Black},
        Color{fg::BrightYellow, bg::Black},
    };

    auto currentColor = titleColors.colorNormalized(0.35);
    terminal.printLine(currentColor, "Animated headline");

``colorNormalized()`` selects a color based on a normalized position in the range ``0.0`` to ``1.0``.
This makes it straightforward to drive color changes using animation cycles, time values, or progress indicators.

.. figure:: demos/images/retro-plasma.jpg
    :width: 100%

    The ``retro-plasma`` demo uses ``ColorSequence`` to switch between
    multiple animated palettes.
