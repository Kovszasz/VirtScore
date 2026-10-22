# Alteration chord notation

An alteration chord is a score annotation with a larger central label and twelve
smaller labels around it. It is a dedicated engraving element derived from
`TextBase`, separate from playable chord symbols, staff-text sound flags, and
figured bass. It creates no notes, MIDI events, or audio links.

## Add and edit

Select a note or rest, then choose **Add → Text → Alteration chord**. The same
command is available from the note-input toolbar's Text menu and the Text
palette. The default central label is `I`; replace it with `IV`, another Roman
numeral, or arbitrary text.

Double-click an existing annotation or select it and press Enter to edit it.
Twelve square placeholders appear while editing. Click a field or use Tab and
Shift+Tab to move through the central text and surrounding fields. The active
field is highlighted. Surrounding fields select their whole value when focused;
typing replaces that selection, then appends further characters. Ctrl/Cmd+A,
copy, cut, and paste act on the active field. Use Backspace to remove text or
Delete to clear a surrounding field. Press Escape or click outside the annotation to finish.
Undo and redo use the score's normal commands.

The surrounding fields are indexed clockwise, starting at the upper-left corner:

```text
             0       1       2       3

            11      central           4
            10       text             5

             9       8       7       6
```

Fields 0, 3, 6, and 9 are the corners. Fields 1–2, 4–5, 7–8, and 10–11 are the
two fields on each edge. Each surrounding label uses 70% of the central font
size. Values such as `3`, `6♯`, and `♭` can reproduce the supplied score example.

Placeholder outlines belong to the editing overlay. They are absent from normal
score rendering, printing, and graphical exports; empty fields draw no text.
Normal annotation placement and dragging apply.

Surrounding fields sit close to the central glyph, with a small gap of 0.05
staff spaces at the sides. Empty squares are smaller, and corner targets move
outward when needed so all twelve fields stay individually selectable. The
special-character toolbar and Unicode/symbol palette insert into the selected
field, replacing selected field text or appending after it. Musical glyphs use
the score's musical text font; letters and numbers keep the annotation text font.


## Storage and compatibility

Scores save the element as `<AlterationChord>` with normal text/engraving
properties and `<slot0>` through `<slot11>` for nonempty surrounding fields:

```xml
<AlterationChord>
  <text>IV</text>
  <slot2>6♯</slot2>
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
enter different values in all twelve fields, leave some empty, reopen the score,
and compare the score with a PDF/SVG export. Check keyboard and mouse field
navigation, copy/paste, and undo after leaving edit mode.

Windows MSI build/download instructions are in [windows-release.md](windows-release.md).
