..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

************
Buffer Views
************

Buffer views expose a rectangular window onto a larger readable buffer.

They are the right tool whenever your logical content is larger than the visible terminal area—for example in scrollable
panels, editors, minimaps, or diagnostic tools.

``BufferViewBase`` provides the shared view behavior, ``BufferView`` owns a shared pointer to the underlying content,
and ``BufferConstRefView`` is the lightweight, stack-friendly variant for temporary rendering.

Usage
=====

Viewing a Portion of a Larger Buffer
------------------------------------

Use a buffer view when you want to render only a specific region of a larger logical canvas.

.. code-block:: cpp

    auto world = Buffer{BlockSize{120, 40}};
    world.fill(Block{" ", Color{fg::Inherited, bg::Black}});
    world.drawBlockText("Visible window", BlockRectangle{10, 6, 20, 3}, Alignment::Center);

    auto view = BufferConstRefView{world, BlockRectangle{8, 4, 40, 12}};

    auto settings = UpdateSettings{};
    settings.setShowCropMarks(true);
    terminal.updateScreen(view, settings);

The view translates its local coordinates into the corresponding positions of the underlying buffer.
This allows you to render just the visible portion without copying or modifying the original content.

Scrolling by Moving the View BlockRectangle
-------------------------------------------

``BufferViewBase`` stores the currently visible rectangle, which you can update as the user scrolls or pans through the
content.

.. code-block:: cpp

    auto sharedWorld = std::make_shared<Buffer>(world);
    auto view = BufferView{sharedWorld, BlockRectangle{0, 0, 40, 12}};

    view.setViewRect(BlockRectangle{16, 10, 40, 12});
    terminal.updateScreen(view);

By moving the view rectangle, you change which part of the content is visible—without copying, reallocating, or
redrawing the underlying buffer.

Choosing Between Shared and Referenced Views
--------------------------------------------

Choose ``BufferView`` when the view needs to outlive the current scope or be stored as part of a larger object.

Choose ``BufferConstRefView`` when you only need a short-lived wrapper around an existing buffer.

.. code-block:: cpp

    auto sharedBuffer = std::make_shared<Buffer>(BlockSize{80, 24});
    auto cachedView = BufferView{sharedBuffer, BlockRectangle{4, 4, 30, 10}};

    auto preview = BufferConstRefView{*sharedBuffer, BlockRectangle{0, 0, 20, 6}};
    terminal.updateScreen(preview);

Both variants implement ``ReadableBuffer``, so the rest of your rendering pipeline can treat them just like any other
source of terminal cells.

Showing Cropped Edges Explicitly
--------------------------------

``CropEdges`` describes which sides of a view are clipped by the available content.

``BufferViewBase`` can use this information to render custom crop indicators directly inside the view.

.. code-block:: cpp

    auto sharedBuffer = std::make_shared<Buffer>(world);
    auto view = BufferView{sharedBuffer, BlockRectangle{8, 4, 40, 12}};

    view.setShowCropCharacters(true);
    view.setCropCharacter(BlockDirection::East, Block{U'▶', fg::BrightYellow});
    view.setCropCharacter(BlockDirection::South, Block{U'▼', fg::BrightYellow});

    const auto cropEdges = CropEdges::fromView(view.viewRect(), sharedBuffer->rect());
    if (cropEdges.isSet(BlockDirection::East)) {
        terminal.printLine("There is more content to the right.");
    }

This is especially helpful in scrollable views, where users should immediately recognize that additional content exists
beyond the visible window.

If you are already using the UI framework, see ``ui::ScrollingBufferView`` for the same concept packaged as a
ready-to-use surface with scroll and page navigation helpers.

