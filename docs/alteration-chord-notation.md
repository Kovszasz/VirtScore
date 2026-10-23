# Alteration chord notation

An alteration chord is a score annotation with a larger central label and ten
smaller labels around it. It is a dedicated engraving element derived from
`TextBase`, separate from playable chord symbols, staff-text sound flags, and
figured bass. It creates no notes, MIDI events, or audio links.

## Add and edit

Select a note or rest, then choose **Add → Text → Alteration chord**. The same
command is available from the note-input toolbar's Text menu and the Text
palette. The default central label is `I`; replace it with `IV`, another Roman
numeral, or arbitrary text.

Double-click an existing annotation or select it and press Enter to edit it.
Ten field outlines appear while editing. Click a field or use Tab and
Shift+Tab to move through the central text and surrounding fields. The active
field is highlighted. Surrounding fields preserve their text when focused. Tab
places the cursor at the end; clicking places it near the clicked character.
Use Left/Right and Home/End to move the cursor, and type or paste to insert at
that position. Ctrl/Cmd+A, copy, cut, and paste act on the active field.
Backspace and Delete remove the character before or after the cursor;
Ctrl/Cmd+A followed by typing replaces the whole field. Press Escape or click
outside the annotation to finish.
Undo and redo use the score's normal commands.

The surrounding fields are indexed clockwise, starting at the upper-left corner:

```text
             0           1           2

             9        central        3
             8         text          4

             7           6           5
```

Fields 0, 2, 5, and 7 are the corners. Fields 1 and 6 are the single centered
fields above and below the main character. Fields 3–4 and 8–9 are the pairs on
the right and left edges. Each surrounding label uses 70% of the central font
size. Values such as `3`, `6♯`, and `♭` can reproduce the supplied score example.

Placeholder outlines belong to the editing overlay. They are absent from normal
score rendering, printing, and graphical exports; empty fields draw no text.
Normal annotation placement and dragging apply.

Surrounding fields sit close to the central glyph, with a small gap of 0.05
staff spaces when space permits. Top labels align at their bottom edge and
bottom labels at their top edge.
Corner boxes share columns with side boxes, and the central box fits the visible
characters closely. Field outlines adapt to their text while keeping all ten
fields individually selectable. The Unicode/symbol palette inserts into the selected field, replacing selected
field text or inserting at the cursor. While editing an alteration chord, the top
note-input toolbar's accidental and articulation buttons insert their glyphs
into the active field instead of applying them to notes. In a side field these
buttons append to the existing number, so `3` followed by the sharp button yields
`3♯`. Keyboard text also supports multiple characters, including `3#`, `5b`,
and printable AltGr combinations. Musical glyphs use
the score's musical text font; letters and numbers keep the annotation text font.


## Storage and compatibility

Scores save the element as `<AlterationChord>` with normal text/engraving
properties. Native XML retains the original field IDs: `slot0`, `slot1`,
`slot3`, `slot4`, `slot5`, `slot6`, `slot8`, `slot9`, `slot10`, and `slot11`.
On opening older scores, the two top values (`slot1` + `slot2`) and the two
bottom values (`slot8` + `slot7`) are joined in visual left-to-right order into
the new top and bottom fields; other positions keep their values:

```xml
<AlterationChord>
  <text>IV</text>
  <slot1>6♯</slot1>
  <slot8>♭</slot8>
</AlterationChord>
```

The current native score reader and writer handle this element, including its
clipboard representation. Existing notation types retain their element IDs.
Older MuseScore versions and forks without this feature do not understand the
new element. Keep an original VirtScore score when sharing an exported PDF or
SVG. MusicXML/MEI interchange of these annotations is not implemented.

## Build and test

Use the existing devcontainer and enable unit tests when configuring it:

```sh
cmake -S . -B build/devcontainer -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DMUSE_ENABLE_UNIT_TESTS=ON
cmake --build build/devcontainer \
  --target engraving_tests notation notationscene palette appshell propertiespanel --parallel 2
(cd build/devcontainer && QT_QPA_PLATFORM=offscreen \
  src/engraving/tests/engraving_tests --gtest_filter='*AlterationChord*')
```

For a full application build, run `cmake --build build/devcontainer --target
MuseScoreStudio --parallel 2`.

The tests are registered in the existing engraving suite and run through the
repository's unit-test workflow. For manual review, add `IV` beneath a staff,
enter different values in all ten fields, leave some empty, reopen the score,
and compare the score with a PDF/SVG export. Check keyboard and mouse field
navigation, copy/paste, and undo after leaving edit mode.

Windows MSI build/download instructions are in [windows-release.md](windows-release.md).
