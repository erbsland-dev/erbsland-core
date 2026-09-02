..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*****************
Paragraph Options
*****************

``ParagraphOptions`` collects the low-level layout rules for wrapped terminal paragraphs.
The same object model is used by ``Terminal::printParagraph()``, ``BlockTextOptions``, and ``BlockText``, so one
paragraph configuration can be reused for direct terminal output and buffer-based text rendering.

.. dropdown:: Details about the example output on this page

    The examples below were rendered with the dedicated documentation helper
    :file:`doc/tools/paragraph-options-reference.cpp` at a fixed width of 78 terminal columns. Each example
    starts with the same ruler line so it is easy to see exactly where the available width ends.

Usage
=====

Building and Reusing Paragraph Presets
--------------------------------------

Keep one configured ``ParagraphOptions`` instance and reuse it for every paragraph with the same layout rules.

.. code-block:: cpp

    auto options = ParagraphOptions{Alignment::TopLeft};
    options.setWrappedLineIndent(8);
    options.setLineBreakStartMark(BlockString{U"⤥"});
    options.setLineBreakEndMark(BlockString{U"⤦"});
    options.setParagraphSpacing(ParagraphSpacing::DoubleLine);
    options.setMaximumLineWraps(2);
    options.setParagraphEllipsisMark(BlockString{"…"});
    options.setTabStops({18, 32, ParagraphOptions::cTabWrappedLineIndent});

    terminal.printParagraph(paragraphText, options);

When the same content is rendered through ``BlockText``, the same settings are available through the forwarding setters
on ``BlockText`` and ``BlockTextOptions``.

Defaults at a Glance
--------------------

.. list-table::
    :header-rows: 1
    :widths: 25 25 50

    *   - Option group
        - Default
        - Purpose
    *   - Alignment
        - ``TopLeft``
        - Left-align text, with top alignment when the paragraph is rendered inside a rectangle.
    *   - Indentation
        - ``0``
        - No base indent, and both special indents inherit from ``lineIndent()``.
    *   - Background fill
        - ``Default``
        - Leave untouched buffer cells outside the visible text.
    *   - Paragraph spacing
        - ``SingleLine``
        - Render explicit newline-separated paragraphs without an extra blank row.
    *   - Word separators
        - ``U"\t "``
        - Split words at spaces and tabs.
    *   - Word break mark
        - ``'-'``
        - Mark a word that was split because it did not fit on one physical line.
    *   - Maximum line wraps
        - ``0``
        - Allow unlimited automatic wraps.
    *   - Paragraph ellipsis mark
        - ``"…"``
        - Mark clipped paragraphs after the configured wrap limit.
    *   - Tab stops
        - ``cTabWrappedLineIndent``
        - Use the wrapped-line indent as the first configured tab stop.
    *   - Tab overflow
        - ``AddSpace``
        - Replace a non-advancing tab with one space.
    *   - Error handling
        - ``PlainOutput``
        - Fall back to plain terminal output if the layout cannot be built.

Alignment
---------

The ``alignment()`` setting controls the horizontal placement of each physical line.
For ``BlockText``, the vertical component of the alignment is also used when the text is rendered inside a rectangle.

Alignment is the first setting most users reach for, so it is worth seeing the three layouts side by side.

**Left**

.. code-block:: cpp

    auto options = ParagraphOptions{};
    options.setAlignment(Alignment::Left);
    terminal.printParagraph(text, options);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97mA paragraph can announce its theme at once: ␛[37mthe winter lantern swung above the␛[39m
    ␛[37mharbor road while clerks, musicians, and late readers hurried homeward beneath␛[39m
    ␛[37mthe same patient rain, each keeping a different pace and yet belonging to the␛[39m 
    ␛[37msame line.␛[39m                                                                    

**Center**

.. code-block:: cpp

    auto options = ParagraphOptions{};
    options.setAlignment(Alignment::Center);
    terminal.printParagraph(text, options);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97mA paragraph can announce its theme at once: ␛[37mthe winter lantern swung above the␛[39m
    ␛[37mharbor road while clerks, musicians, and late readers hurried homeward beneath␛[39m
    ␛[37mthe same patient rain, each keeping a different pace and yet belonging to the␛[39m 
                                      ␛[37msame line.␛[39m                                  

**Right**

.. code-block:: cpp

    auto options = ParagraphOptions{};
    options.setAlignment(Alignment::Right);
    terminal.printParagraph(text, options);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97mA paragraph can announce its theme at once: ␛[37mthe winter lantern swung above the␛[39m
    ␛[37mharbor road while clerks, musicians, and late readers hurried homeward beneath␛[39m
     ␛[37mthe same patient rain, each keeping a different pace and yet belonging to the␛[39m
                                                                        ␛[37msame line.␛[39m

Indentation
-----------

Indentation is only applied for left-aligned paragraphs.
The three related settings work as a small inheritance chain:

* ``lineIndent()`` is the base indent for all physical lines.
* ``firstLineIndent()`` overrides only the first physical line.
* ``wrappedLineIndent()`` overrides only wrapped continuation lines.
* ``cUseLineIndent`` means "reuse ``lineIndent()`` here".

**lineIndent = 8**

.. code-block:: cpp

    auto options = ParagraphOptions{};
    options.setLineIndent(8);
    terminal.printParagraph(text, options);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m        ␛[97mIndented paragraphs are excellent for guidance text: ␛[37mthey let a short␛[39m 
            ␛[37mheading stand close to the margin while the calmer explanatory part␛[39m   
            ␛[37msettles slightly deeper, which keeps option descriptions, notes, and␛[39m  
            ␛[37mexamples easy to scan in a crowded terminal.␛[39m                          

**firstLineIndent = 0**

.. code-block:: cpp

    auto options = ParagraphOptions{};
    options.setLineIndent(8);
    options.setFirstLineIndent(0);
    terminal.printParagraph(text, options);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97mIndented paragraphs are excellent for guidance text: ␛[37mthey let a short heading␛[39m 
            ␛[37mstand close to the margin while the calmer explanatory part settles␛[39m   
            ␛[37mslightly deeper, which keeps option descriptions, notes, and examples␛[39m 
            ␛[37measy to scan in a crowded terminal.␛[39m                                   

**wrappedLineIndent = 14**

.. code-block:: cpp

    auto options = ParagraphOptions{};
    options.setLineIndent(8);
    options.setWrappedLineIndent(14);
    terminal.printParagraph(textWithNewline, options);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m        ␛[97mFirst line: ␛[37mA wrapped continuation should move to the deeper␛[39m          
                  ␛[37mcontinuation indent so the reader can tell that the sentence is␛[39m 
                  ␛[37mstill flowing forward.␛[39m                                          
            ␛[97mAfter newline: ␛[37mA hard line break starts again at the normal line␛[39m      
                  ␛[37mindent before any later wraps move back to the deeper␛[39m           
                  ␛[37mwrapped-line indent.␛[39m                                            

Wrap Marks
----------

``lineBreakStartMark()`` and ``lineBreakEndMark()`` decorate automatic wraps.

Use them when you want readers to immediately see where a continuation begins and where the previous line ran out of
space.

**lineBreakStartMark = "⤥"**

.. code-block:: cpp

    auto options = ParagraphOptions{};
    options.setLineBreakStartMark(BlockString{U"⤥"});
    terminal.printParagraph(text, options);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97mVisible wrap markers turn layout into something the reader can trust: ␛[37mthey␛[39m    
    ⤥␛[37mshow exactly where the sentence continues, which is especially helpful in␛[39m    
    ⤥␛[37mpreviews, manuals, and teaching material where the shape of the paragraph␛[39m    
    ⤥␛[37mmatters as much as the words.␛[39m                                                

**lineBreakEndMark = "⤦"**

.. code-block:: cpp

    auto options = ParagraphOptions{};
    options.setLineBreakEndMark(BlockString{U"⤦"});
    terminal.printParagraph(text, options);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97mVisible wrap markers turn layout into something the reader can trust: ␛[37mthey␛[39m   ⤦
    ␛[37mshow exactly where the sentence continues, which is especially helpful in␛[39m    ⤦
    ␛[37mpreviews, manuals, and teaching material where the shape of the paragraph␛[39m    ⤦
    ␛[37mmatters as much as the words.␛[39m                                                 

**both marks with wrappedLineIndent = 8**

.. code-block:: cpp

    auto options = ParagraphOptions{};
    options.setWrappedLineIndent(8);
    options.setLineBreakStartMark(BlockString{U"⤥"});
    options.setLineBreakEndMark(BlockString{U"⤦"});
    terminal.printParagraph(text, options);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97mVisible wrap markers turn layout into something the reader can trust: ␛[37mthey␛[39m   ⤦
            ⤥␛[37mshow exactly where the sentence continues, which is especially␛[39m      ⤦
            ⤥␛[37mhelpful in previews, manuals, and teaching material where the shape␛[39m ⤦
            ⤥␛[37mof the paragraph matters as much as the words.␛[39m                       

Paragraph Spacing
-----------------

``ParagraphSpacing`` controls the vertical gap between explicit paragraphs.

For ``BlockText``, each newline starts a new paragraph.
For ``Terminal::printParagraph()``, paragraph spacing is applied after the call finishes.

**ParagraphSpacing::SingleLine**

.. code-block:: cpp

    auto text = BlockText{paragraphs, buffer.rect(), Alignment::TopLeft};
    text.setParagraphSpacing(ParagraphSpacing::SingleLine);
    buffer.drawBlockText(text);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97mSynopsis: ␛[37mThe watchman trimmed the lamp, checked the gate, and listened for␛[39m   
    ␛[37mthe returning carriage.␛[39m                                                       
    ␛[97mExamples: ␛[37mUse double spacing when neighboring paragraphs should read like␛[39m     
    ␛[37mseparate steps instead of one continuous argument.␛[39m                            

**ParagraphSpacing::DoubleLine**

.. code-block:: cpp

    auto text = BlockText{paragraphs, buffer.rect(), Alignment::TopLeft};
    text.setParagraphSpacing(ParagraphSpacing::DoubleLine);
    buffer.drawBlockText(text);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97mSynopsis: ␛[37mThe watchman trimmed the lamp, checked the gate, and listened for␛[39m   
    ␛[37mthe returning carriage.␛[39m                                                       
                                                                                  
    ␛[97mExamples: ␛[37mUse double spacing when neighboring paragraphs should read like␛[39m     
    ␛[37mseparate steps instead of one continuous argument.␛[39m                            

Word Separators
---------------

``wordSeparators()`` defines which characters split a source line into words.
Consecutive separators collapse into one rendered space.

**Default separators**

.. code-block:: cpp

    auto text = BlockText{pathLikeText, buffer.rect(), Alignment::TopLeft};
    buffer.drawBlockText(text);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97mdocs/reference/paragraph-options/with/illustrated/examples/for/layout/choices␛[39m-
    ␛[97m/␛[37mand/friendly/terminal/output/that/readers/can/skim/without/guesswork␛[39m         
                                                                                  

**wordSeparators = U" /"**

.. code-block:: cpp

    auto text = BlockText{pathLikeText, buffer.rect(), Alignment::TopLeft};
    text.setWordSeparators(U" /");
    buffer.drawBlockText(text);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97mdocs reference paragraph-options with illustrated examples for layout choices␛[39m 
    ␛[37mand friendly terminal output that readers can skim without guesswork␛[39m          
                                                                                  

Word Break Mark
---------------

If a single word still does not fit, the layout engine splits it at display-cell boundaries and appends
``wordBreakMark()`` to each wrapped chunk except the last one.

**Default '-'**

.. code-block:: cpp

    auto text = BlockText{longIdentifier, buffer.rect(), Alignment::TopLeft};
    buffer.drawBlockText(text);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97mParagraphLayoutDemonstrationIdentifierForReadersWhoPrefer␛[37mVeryLongNamesThatSti␛[39m-
    ␛[37mllNeedPredictableWrappingInReferenceManualsAndTerminalPreviews␛[39m                
                                                                                  

**wordBreakMark = '~'**

.. code-block:: cpp

    auto text = BlockText{longIdentifier, buffer.rect(), Alignment::TopLeft};
    text.setWordBreakMark(Block{U'~'});
    buffer.drawBlockText(text);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97mParagraphLayoutDemonstrationIdentifierForReadersWhoPrefer␛[37mVeryLongNamesThatSti␛[39m~
    ␛[37mllNeedPredictableWrappingInReferenceManualsAndTerminalPreviews␛[39m                
                                                                                  

Maximum Wraps and Ellipsis
--------------------------

``maximumLineWraps()`` limits how many automatic wraps a single source line may produce.
Once the limit is reached, the paragraph stops and appends ``paragraphEllipsisMark()``.

**Unlimited wraps**

.. code-block:: cpp

    auto text = BlockText{summaryText, buffer.rect(), Alignment::TopLeft};
    buffer.drawBlockText(text);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97mSometimes a paragraph should stop politely instead of taking over the screen:␛[39m 
    ␛[37mfor release notes, narrow side panels, or compact popovers, a short ellipsis␛[39m  
    ␛[37mcan admit that more text exists without forcing the entire chapter into a␛[39m     
    ␛[37mspace meant for a summary.␛[39m                                                    

**maximumLineWraps = 1**

.. code-block:: cpp

    auto text = BlockText{summaryText, buffer.rect(), Alignment::TopLeft};
    text.setMaximumLineWraps(1);
    text.setParagraphEllipsisMark(BlockString{" (more)"});
    buffer.drawBlockText(text);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97mSometimes a paragraph should stop politely instead of taking over the screen:␛[39m 
    ␛[37mfor release notes, narrow side panels, or compact popovers, a short␛[39m (more)    

Tab Stops and Overflow Behavior
-------------------------------

Tabs receive special handling only in left-aligned paragraphs.
In centered or right-aligned paragraphs, tabs are handled through ``wordSeparators()``.

**tabStops = {14}**

.. code-block:: cpp

    auto text = BlockText{tabbedRows, buffer.rect(), Alignment::TopLeft};
    text.setTabStops({14});
    buffer.drawBlockText(text);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97mbuild␛[37m         Compile the project tree and refresh the generated headers␛[39m      
    ␛[97mtest␛[37m          Run the unit tests and inspect the failing cases while the logs␛[39m 
    ␛[37mare still fresh␛[39m                                                               
    ␛[97mpublish␛[37m       Create the release archive and attach the changelog for the␛[39m     

**TabOverflowBehavior::AddSpace**

.. code-block:: cpp

    auto text = BlockText{tableLikeRows, buffer.rect(), Alignment::TopLeft};
    text.setTabStops({10, 20, 30, 40, 50, 60, 70});
    text.setTabOverflowBehavior(TabOverflowBehavior::AddSpace);
    buffer.drawBlockText(text);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97mName␛[37m      Start     Middle    Finish    Notes     Owner     State␛[39m             
    ␛[97mRiver␛[37m     Stone     Candlelight Map     Ink       Rope      Ready␛[39m             
    ␛[97mHarbor␛[37m    Lantern   Weatherproof Clock  Seal      Ledger    Waiting␛[39m           
    ␛[97mGarden␛[37m    Gate      Silverthread Bell   Twine     Packet    Queued␛[39m            

**TabOverflowBehavior::LineBreak**

.. code-block:: cpp

    auto text = BlockText{optionDescriptions, buffer.rect(), Alignment::TopLeft};
    text.setWrappedLineIndent(15);
    text.setTabStops({15});
    text.setTabOverflowBehavior(TabOverflowBehavior::LineBreak);
    buffer.drawBlockText(text);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97m--color␛[37m        Choose the accent colors for the preview panels and the footer␛[39m 
                   ␛[37mhints␛[39m                                                          
    ␛[97m--maximum-description-column␛[39m                                                  
                   ␛[37mCap the description tab stop so narrow terminals still wrap␛[39m    
                   ␛[37mcleanly␛[39m                                                        
    ␛[97m--paragraph-ellipsis-mark␛[39m                                                     
                   ␛[37mShow a compact marker when the preview summary had to be␛[39m       
                   ␛[37mclipped␛[39m                                                        

Background Fill Modes
---------------------

``ParagraphBackgroundMode`` controls whether the background color of the last visible character is extended into cells
that do not contain visible text.

These modes behave slightly differently depending on the rendering path:

* With ``BlockText`` and
  ``WritableBuffer::drawBlockText()``, untouched cells remain whatever
  is already stored in the target buffer unless the selected mode fills them from the wrapped text.
* With ``CursorWriter::printParagraph()``, indentation and
  trailing padding must be written as space characters. Cells not filled from wrapped-text background therefore use
  the writer's current background color.

If paragraph padding should blend into a panel when using ``CursorWriter::printParagraph()``, set the writer background
before printing the paragraph.

The following examples use buffer-based text rendering.
The paragraph is blue and the surrounding buffer is dark grey.

**ParagraphBackgroundMode::Default**

In buffer-based rendering, the continuation indent and the right side keep the existing buffer background.
With ``CursorWriter::printParagraph()``, the same cells use the current writer background instead.

.. code-block:: cpp

    auto text = BlockText{paragraph, buffer.rect(), Alignment::TopLeft};
    text.setWrappedLineIndent(10);
    text.setBackgroundMode(ParagraphBackgroundMode::Default);
    buffer.drawBlockText(text);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97;44mBackground fill is easier to judge with a sentence that keeps moving: ␛[37mthe blue␛[39;49m
    ␛[97m          ␛[37;44mpaper lantern glowed at the window while the final line faded into␛[97;100m  ␛[39;49m
    ␛[97m          ␛[37;44mthe dim grey hall beyond it.␛[97;100m                                        ␛[39;49m

**ParagraphBackgroundMode::WrappedLeft**

.. code-block:: cpp

    auto text = BlockText{paragraph, buffer.rect(), Alignment::TopLeft};
    text.setWrappedLineIndent(10);
    text.setBackgroundMode(ParagraphBackgroundMode::WrappedLeft);
    buffer.drawBlockText(text);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97;44mBackground fill is easier to judge with a sentence that keeps moving: ␛[37mthe blue␛[39;49m
    ␛[97;44m          ␛[37mpaper lantern glowed at the window while the final line faded into␛[97;100m  ␛[39;49m
    ␛[97;44m          ␛[37mthe dim grey hall beyond it.␛[97;100m                                        ␛[39;49m

**ParagraphBackgroundMode::WrappedRight**

The continuation indent still keeps the existing buffer background in the examples below.
When printed through ``CursorWriter::printParagraph()``, it uses the current writer background instead.

.. code-block:: cpp

    auto text = BlockText{paragraph, buffer.rect(), Alignment::TopLeft};
    text.setWrappedLineIndent(10);
    text.setBackgroundMode(ParagraphBackgroundMode::WrappedRight);
    buffer.drawBlockText(text);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97;44mBackground fill is easier to judge with a sentence that keeps moving: ␛[37mthe blue␛[39;49m
    ␛[97m          ␛[37;44mpaper lantern glowed at the window while the final line faded into␛[97m  ␛[39;49m
    ␛[97m          ␛[37;44mthe dim grey hall beyond it.␛[97;100m                                        ␛[39;49m

**ParagraphBackgroundMode::WrappedBoth**

.. code-block:: cpp

    auto text = BlockText{paragraph, buffer.rect(), Alignment::TopLeft};
    text.setWrappedLineIndent(10);
    text.setBackgroundMode(ParagraphBackgroundMode::WrappedBoth);
    buffer.drawBlockText(text);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97;44mBackground fill is easier to judge with a sentence that keeps moving: ␛[37mthe blue␛[39;49m
    ␛[97;44m          ␛[37mpaper lantern glowed at the window while the final line faded into␛[97m  ␛[39;49m
    ␛[97;44m          ␛[37mthe dim grey hall beyond it.␛[97;100m                                        ␛[39;49m

**ParagraphBackgroundMode::FullRight**

.. code-block:: cpp

    auto text = BlockText{paragraph, buffer.rect(), Alignment::TopLeft};
    text.setWrappedLineIndent(10);
    text.setBackgroundMode(ParagraphBackgroundMode::FullRight);
    buffer.drawBlockText(text);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97;44mBackground fill is easier to judge with a sentence that keeps moving: ␛[37mthe blue␛[39;49m
    ␛[97m          ␛[37;44mpaper lantern glowed at the window while the final line faded into␛[97m  ␛[39;49m
    ␛[97m          ␛[37;44mthe dim grey hall beyond it.␛[97m                                        ␛[39;49m

**ParagraphBackgroundMode::FullBoth**

.. code-block:: cpp

    auto text = BlockText{paragraph, buffer.rect(), Alignment::TopLeft};
    text.setWrappedLineIndent(10);
    text.setBackgroundMode(ParagraphBackgroundMode::FullBoth);
    buffer.drawBlockText(text);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97;44mBackground fill is easier to judge with a sentence that keeps moving: ␛[37mthe blue␛[39;49m
    ␛[97;44m          ␛[37mpaper lantern glowed at the window while the final line faded into␛[97m  ␛[39;49m
    ␛[97;44m          ␛[37mthe dim grey hall beyond it.␛[97m                                        ␛[39;49m

Reusable Indents and Separator Sets
-----------------------------------

``ParagraphIndents`` and ``FastCharSet`` are useful when you want to reuse the same low-level paragraph mechanics across
several rendering presets.

.. code-block:: cpp

    auto indents = ParagraphIndents{2};
    indents.setFirstLineIndent(0);
    indents.setWrappedLineIndent(6);
    indents.setMargins(Margins{1, 1, 0, 1});

    auto options = ParagraphOptions{};
    options.setIndents(indents);
    options.setWordSeparatorSet(FastCharSet::create(U" \t/"));

    terminal.printParagraph("path/to/the/current/document section", options);

``FastCharSet`` keeps separator lookups cheap when the same character set is reused across many paragraphs.
The shared factories ``FastCharSet::onlySpace()`` and ``FastCharSet::spaceAndTab()`` cover the most common cases
directly.

Error Handling
--------------

Some combinations of width and paragraph settings are impossible to render.
Typical causes are:

* wrap markers or ellipsis markers that leave no room for visible text
* indentation that consumes the whole available width
* a very narrow rectangle combined with aggressive wrap decoration

``ParagraphOnError`` controls the fallback.

**ParagraphOnError::PlainOutput**

.. code-block:: cpp

    auto text = BlockText{BlockString{"AA BB"}, Rectangle{0, 0, 2, 2}, Alignment::TopLeft};
    text.setLineBreakEndMark(BlockString{U"⤦⤦"});
    text.setOnError(ParagraphOnError::PlainOutput);
    buffer.drawBlockText(text);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[97mAA␛[39m                                                                            
    ␛[37mBB␛[39m                                                                            

**ParagraphOnError::Empty**

.. code-block:: cpp

    auto text = BlockText{BlockString{"AA BB"}, Rectangle{0, 0, 2, 2}, Alignment::TopLeft};
    text.setLineBreakEndMark(BlockString{U"⤦⤦"});
    text.setOnError(ParagraphOnError::Empty);
    buffer.drawBlockText(text);

.. erbsland-ansi::
    :escape-char: ␛

    ␛[90m012345678901234567890123456789012345678901234567890123456789012345678901234567
    ␛[39m␛[90m··············································································␛[39m
    ␛[90m··············································································␛[39m

