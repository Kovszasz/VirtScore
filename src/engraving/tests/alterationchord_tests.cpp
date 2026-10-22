/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>

#include "engraving/compat/scoreaccess.h"
#include "engraving/dom/alterationchord.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/chordrest.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/note.h"
#include "engraving/dom/part.h"
#include "engraving/dom/rest.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/symbol.h"
#include "engraving/iengravingfont.h"
#include "engraving/editing/editdata.h"
#include "engraving/editing/textedit.h"
#include "engraving/editing/transaction/transaction.h"
#include "engraving/editing/transaction/undostack.h"
#include "engraving/rendering/iscorerenderer.h"
#include "draw/fontmetrics.h"
#include "draw/types/transform.h"
#include "utils/scorerw.h"

using namespace mu::engraving;

class Engraving_AlterationChordTests : public ::testing::Test
{
protected:
    MasterScore* m_score = nullptr;

    void SetUp() override
    {
        m_score = compat::ScoreAccess::createMasterScore(nullptr);
        Part* part = new Part(m_score);
        m_score->appendPart(part);
        Staff* staff = Factory::createStaff(part);
        m_score->appendStaff(staff);
    }

    void TearDown() override
    {
        delete m_score;
    }
};

TEST_F(Engraving_AlterationChordTests, DefaultsAndCreation)
{
    AlterationChord* chord = Factory::createAlterationChord(m_score->dummy());
    ASSERT_NE(chord, nullptr);
    EXPECT_EQ(chord->type(), ElementType::ALTERATION_CHORD);
    EXPECT_EQ(chord->centralText(), u"I");
    EXPECT_EQ(chord->activeSlot(), AlterationChord::FOCUS_CENTRAL);
    EXPECT_TRUE(chord->positionRelativeToNoteheadRest());

    for (int i = 0; i < AlterationChord::SLOTS_COUNT; ++i) {
        EXPECT_TRUE(chord->slotText(i).isEmpty());
    }

    delete chord;
}

TEST_F(Engraving_AlterationChordTests, SlotAccessorsAndSetters)
{
    AlterationChord* chord = Factory::createAlterationChord(m_score->dummy());
    ASSERT_NE(chord, nullptr);

    chord->setCentralText(u"IV");
    EXPECT_EQ(chord->centralText(), u"IV");

    chord->setSlotText(0, u"b9");
    chord->setSlotText(3, u"6#");
    chord->setSlotText(7, u"b");
    chord->setSlotText(11, u"3");

    EXPECT_EQ(chord->slotText(0), u"b9");
    EXPECT_EQ(chord->slotText(3), u"6#");
    EXPECT_EQ(chord->slotText(7), u"b");
    EXPECT_EQ(chord->slotText(11), u"3");

    // Out of bounds check
    EXPECT_TRUE(chord->slotText(-1).isEmpty());
    EXPECT_TRUE(chord->slotText(12).isEmpty());

    delete chord;
}

TEST_F(Engraving_AlterationChordTests, NavigationTraversal)
{
    AlterationChord* chord = Factory::createAlterationChord(m_score->dummy());
    ASSERT_NE(chord, nullptr);

    EXPECT_EQ(chord->activeSlot(), AlterationChord::FOCUS_CENTRAL);

    // Forward cycle with nextSlot()
    for (int i = 0; i < AlterationChord::SLOTS_COUNT; ++i) {
        chord->nextSlot();
        EXPECT_EQ(chord->activeSlot(), i);
    }
    chord->nextSlot();
    EXPECT_EQ(chord->activeSlot(), AlterationChord::FOCUS_CENTRAL);

    // Backward cycle with prevSlot()
    for (int i = AlterationChord::SLOTS_COUNT - 1; i >= 0; --i) {
        chord->prevSlot();
        EXPECT_EQ(chord->activeSlot(), i);
    }
    chord->prevSlot();
    EXPECT_EQ(chord->activeSlot(), AlterationChord::FOCUS_CENTRAL);

    delete chord;
}

TEST_F(Engraving_AlterationChordTests, EditingKeystrokes)
{
    AlterationChord* chord = Factory::createAlterationChord(m_score->dummy());
    ASSERT_NE(chord, nullptr);

    EditData ed;
    ed.key = Key_Tab;
    chord->edit(ed);
    EXPECT_EQ(chord->activeSlot(), 0);

    // Type character "7" into slot 0
    ed.key = 0;
    ed.s = u"7";
    chord->edit(ed);
    EXPECT_EQ(chord->slotText(0), u"7");

    // Append "#" into slot 0
    ed.s = u"#";
    chord->edit(ed);
    EXPECT_EQ(chord->slotText(0), u"7#");

    // Backspace removes "#"
    ed.key = Key_Backspace;
    ed.s = String();
    chord->edit(ed);
    EXPECT_EQ(chord->slotText(0), u"7");

    // Delete clears the slot
    ed.key = Key_Delete;
    chord->edit(ed);
    EXPECT_TRUE(chord->slotText(0).isEmpty());

    // Shift+Tab traverses backwards to FOCUS_CENTRAL
    ed.key = Key_Backtab;
    chord->edit(ed);
    EXPECT_EQ(chord->activeSlot(), AlterationChord::FOCUS_CENTRAL);

    delete chord;
}

TEST_F(Engraving_AlterationChordTests, UndoRedoSlotText)
{
    AlterationChord* chord = Factory::createAlterationChord(m_score->dummy());
    ASSERT_NE(chord, nullptr);

    m_score->startCmd(TranslatableString::untranslatable("Test"));
    chord->undoSetSlotText(2, u"b5");
    EXPECT_EQ(chord->slotText(2), u"b5");
    m_score->endCmd();

    m_score->undoStack()->undo(nullptr);
    EXPECT_TRUE(chord->slotText(2).isEmpty());

    m_score->undoStack()->redo();
    EXPECT_EQ(chord->slotText(2), u"b5");

    delete chord;
}

TEST_F(Engraving_AlterationChordTests, LayoutGeometry)
{
    AlterationChord* chord = Factory::createAlterationChord(m_score->dummy());
    ASSERT_NE(chord, nullptr);

    AlterationChord::LayoutData* ldata = chord->mutldata();
    double sp = 10.0;
    AlterationChord::calculateSlotsLayout(ldata, sp);

    EXPECT_GE(ldata->slotSize, 0.65 * sp);
    EXPECT_GT(ldata->centralRect.width(), 0.0);
    EXPECT_GE(ldata->centralRect.height(), 2.0 * ldata->slotSize);

    // Verify 4 top slots have identical Y
    for (int i = 0; i < 4; ++i) {
        EXPECT_DOUBLE_EQ(ldata->slotRect[i].y(), ldata->centralRect.top() - ldata->slotSize);
        EXPECT_DOUBLE_EQ(ldata->slotRect[i].height(), ldata->slotSize);
        EXPECT_DOUBLE_EQ(ldata->slotRect[i].width(), ldata->slotSize);
    }

    // Verify 4 bottom slots have identical Y
    for (int i = 6; i <= 9; ++i) {
        EXPECT_DOUBLE_EQ(ldata->slotRect[i].y(), ldata->centralRect.bottom());
        EXPECT_DOUBLE_EQ(ldata->slotRect[i].height(), ldata->slotSize);
        EXPECT_DOUBLE_EQ(ldata->slotRect[i].width(), ldata->slotSize);
    }

    // Left slots have identical X
    EXPECT_DOUBLE_EQ(ldata->slotRect[10].x(), ldata->centralRect.left() - ldata->slotSize);
    EXPECT_DOUBLE_EQ(ldata->slotRect[11].x(), ldata->centralRect.left() - ldata->slotSize);

    // Right slots have identical X
    EXPECT_DOUBLE_EQ(ldata->slotRect[4].x(), ldata->centralRect.right());
    EXPECT_DOUBLE_EQ(ldata->slotRect[5].x(), ldata->centralRect.right());

    // Corner slots checks
    EXPECT_LE(ldata->slotRect[0].right(), ldata->slotRect[1].left()); // TopLeft
    EXPECT_GE(ldata->slotRect[3].left(), ldata->slotRect[2].right()); // TopRight
    EXPECT_GE(ldata->slotRect[6].left(), ldata->slotRect[7].right()); // BottomRight
    EXPECT_LE(ldata->slotRect[9].right(), ldata->slotRect[8].left()); // BottomLeft

    delete chord;
}

TEST_F(Engraving_AlterationChordTests, SerializationRoundTrip)
{
    AlterationChord* chord = Factory::createAlterationChord(m_score->dummy());
    ASSERT_NE(chord, nullptr);

    chord->setCentralText(u"IV");
    chord->setSlotText(0, u"3");
    chord->setSlotText(3, u"6#");
    chord->setSlotText(6, u"b");
    chord->setSlotText(9, u"7");

    AlterationChord* readChord = toAlterationChord(ScoreRW::writeReadElement(chord));
    ASSERT_NE(readChord, nullptr);

    EXPECT_EQ(readChord->type(), ElementType::ALTERATION_CHORD);
    EXPECT_EQ(readChord->centralText(), u"IV");
    EXPECT_EQ(readChord->slotText(0), u"3");
    EXPECT_EQ(readChord->slotText(3), u"6#");
    EXPECT_EQ(readChord->slotText(6), u"b");
    EXPECT_EQ(readChord->slotText(9), u"7");

    // Empty slots remain empty
    EXPECT_TRUE(readChord->slotText(1).isEmpty());
    EXPECT_TRUE(readChord->slotText(2).isEmpty());
    EXPECT_TRUE(readChord->slotText(4).isEmpty());
    EXPECT_TRUE(readChord->slotText(5).isEmpty());
    EXPECT_TRUE(readChord->slotText(7).isEmpty());
    EXPECT_TRUE(readChord->slotText(8).isEmpty());
    EXPECT_TRUE(readChord->slotText(10).isEmpty());
    EXPECT_TRUE(readChord->slotText(11).isEmpty());

    delete readChord;
    delete chord;
}

TEST_F(Engraving_AlterationChordTests, ScoreAttachment)
{
    Measure* m = Factory::createMeasure(m_score);
    m_score->measures()->append(m);

    Segment* seg = m->getSegment(SegmentType::ChordRest, Fraction(0, 1));
    ASSERT_NE(seg, nullptr);

    m_score->startCmd(TranslatableString::untranslatable("Add Alteration Chord"));
    AlterationChord* chord = m_score->addAlterationChord(seg);
    ASSERT_NE(chord, nullptr);
    EXPECT_EQ(chord->segment(), seg);

    // Verify chord is actually present in seg->annotations()
    const auto& annotations = seg->annotations();
    bool present = (std::find(annotations.begin(), annotations.end(), chord) != annotations.end());
    EXPECT_TRUE(present);
    m_score->endCmd();

    m_score->undoStack()->undo(nullptr);
    const auto& annotationsAfterUndo = seg->annotations();
    EXPECT_TRUE(std::find(annotationsAfterUndo.begin(), annotationsAfterUndo.end(), chord) == annotationsAfterUndo.end());

    m_score->undoStack()->redo();
    const auto& annotationsAfterRedo = seg->annotations();
    EXPECT_TRUE(std::find(annotationsAfterRedo.begin(), annotationsAfterRedo.end(), chord) != annotationsAfterRedo.end());
}

TEST_F(Engraving_AlterationChordTests, PaletteDropOnChordRestNoteMeasure)
{
    MasterScore* fixtureScore = ScoreRW::readScore(u"test.mscx");
    ASSERT_NE(fixtureScore, nullptr);
    fixtureScore->doLayout();

    Measure* m = fixtureScore->firstMeasure();
    ASSERT_NE(m, nullptr);
    Segment* s = m->first(SegmentType::ChordRest);
    ASSERT_NE(s, nullptr);
    ChordRest* cr = toChordRest(s->element(0));
    ASSERT_NE(cr, nullptr);

    // 1. Drop on ChordRest
    fixtureScore->transactionManager()->transaction(TranslatableString::untranslatable("Drop on ChordRest"), [&](Transaction& tx) {
        EditData ed(0);
        AlterationChord* newChord = Factory::createAlterationChord(fixtureScore->dummy());
        newChord->setCentralText(u"V");
        newChord->setSlotText(1, u"b9");
        ed.dropElement = newChord;

        EXPECT_TRUE(cr->acceptDrop(ed));
        EngravingItem* dropped = cr->drop(tx, ed);
        ASSERT_NE(dropped, nullptr);
        EXPECT_TRUE(dropped->isAlterationChord());

        const auto& annos = s->annotations();
        EXPECT_TRUE(std::find(annos.begin(), annos.end(), dropped) != annos.end());
    });

    // 2. Drop on Note if Chord has note
    if (cr->isChord()) {
        Note* note = toChord(cr)->upNote();
        ASSERT_NE(note, nullptr);
        fixtureScore->transactionManager()->transaction(TranslatableString::untranslatable("Drop on Note"), [&](Transaction& tx) {
            EditData ed(0);
            AlterationChord* newChord = Factory::createAlterationChord(fixtureScore->dummy());
            ed.dropElement = newChord;

            EXPECT_TRUE(note->acceptDrop(ed));
            EngravingItem* dropped = note->drop(tx, ed);
            ASSERT_NE(dropped, nullptr);
            EXPECT_TRUE(dropped->isAlterationChord());
        });
    }

    // 3. Drop on Measure
    fixtureScore->transactionManager()->transaction(TranslatableString::untranslatable("Drop on Measure"), [&](Transaction& tx) {
        EditData ed(0);
        ed.track = 0;
        AlterationChord* newChord = Factory::createAlterationChord(fixtureScore->dummy());
        ed.dropElement = newChord;

        EXPECT_TRUE(m->acceptDrop(ed));
        EngravingItem* dropped = m->drop(tx, ed);
        ASSERT_NE(dropped, nullptr);
        EXPECT_TRUE(dropped->isAlterationChord());
    });

    delete fixtureScore;
}

TEST_F(Engraving_AlterationChordTests, ClickEmptySlotWithNonzeroCanvasPosAndAngle)
{
    Measure* m = Factory::createMeasure(m_score);
    m_score->measures()->append(m);
    Segment* seg = m->getSegment(SegmentType::ChordRest, Fraction(0, 1));
    ASSERT_NE(seg, nullptr);

    AlterationChord* chord = Factory::createAlterationChord(seg);
    chord->setAutoplace(false);
    chord->setTextAngle(0.0);
    m_score->renderer()->layoutItem(static_cast<TextBase*>(chord));
    chord->setPos(PointF(120.0, 240.0));
    EXPECT_FALSE(chord->canvasPos().isNull());

    const AlterationChord::LayoutData* l = chord->ldata();
    ASSERT_NE(l, nullptr);

    // Target empty slot 3 (TopRight) in CANVAS coords
    PointF slot3Canvas = chord->canvasPos() + l->slotRect[3].center();

    EditData ed;
    ed.startMove = slot3Canvas;
    chord->startEdit(ed);

    EXPECT_EQ(chord->activeSlot(), 3);
    EXPECT_TRUE(chord->isSlotSelected());

    // Click slot 7 (Bottom2) in CANVAS coords via mousePress
    PointF slot7Canvas = chord->canvasPos() + l->slotRect[7].center();
    ed.startMove = slot7Canvas;
    EXPECT_TRUE(chord->mousePress(ed));
    EXPECT_EQ(chord->activeSlot(), 7);
    EXPECT_TRUE(chord->isSlotSelected());

    chord->endEdit(ed);

    // Now test with rotated textAngle
    chord->setTextAngle(90.0);
    m_score->renderer()->layoutItem(static_cast<TextBase*>(chord));
    chord->setPos(PointF(120.0, 240.0));
    const AlterationChord::LayoutData* lRot = chord->ldata();

    // Map slot 1 local center by 90 degrees rotation
    muse::draw::Transform rot;
    rot.rotate(90.0);
    PointF slot1CanvasRot = chord->canvasPos() + rot.map(lRot->slotRect[1].center());
    ed.startMove = slot1CanvasRot;
    EXPECT_TRUE(chord->mousePress(ed));
    EXPECT_EQ(chord->activeSlot(), 1);

    delete chord;
}

TEST_F(Engraving_AlterationChordTests, FullSessionUndoRedo)
{
    Measure* m = Factory::createMeasure(m_score);
    m_score->measures()->append(m);
    Segment* seg = m->getSegment(SegmentType::ChordRest, Fraction(0, 1));
    ASSERT_NE(seg, nullptr);

    m_score->startCmd(TranslatableString::untranslatable("Add chord"));
    AlterationChord* chord = m_score->addAlterationChord(seg);
    m_score->endCmd();
    ASSERT_NE(chord, nullptr);

    EditData ed;
    chord->startEdit(ed);

    // Tab to slot 0
    ed.key = Key_Tab;
    chord->edit(ed);
    EXPECT_EQ(chord->activeSlot(), 0);

    // Type "b9" inside transaction
    m_score->startCmd(TranslatableString::untranslatable("Keystroke"));
    ed.key = 0;
    ed.s = u"b9";
    chord->edit(ed);
    m_score->endCmd();

    EXPECT_EQ(chord->slotText(0), u"b9");

    chord->endEdit(ed);
    EXPECT_EQ(chord->slotText(0), u"b9");

    // Verify changesInfo does not crash on transaction with slot edits
    ASSERT_NE(m_score->undoStack()->last(), nullptr);
    EXPECT_NO_THROW({
        auto infoUndo = m_score->undoStack()->last()->changesInfo(true);
        auto infoRedo = m_score->undoStack()->last()->changesInfo(false);
        (void)infoUndo;
        (void)infoRedo;
    });

    // Undoing the edit session should undo the slot change
    m_score->undoStack()->undo(nullptr);
    EXPECT_TRUE(chord->slotText(0).isEmpty());

    // Redoing should restore "b9"
    m_score->undoStack()->redo();
    EXPECT_EQ(chord->slotText(0), u"b9");
}

TEST_F(Engraving_AlterationChordTests, LinkedCloneSlotEditsAcrossParts)
{
    Part* part2 = new Part(m_score);
    m_score->appendPart(part2);
    Staff* staff2 = Factory::createStaff(part2);
    m_score->appendStaff(staff2);

    AlterationChord* chord1 = Factory::createAlterationChord(m_score->dummy());
    chord1->setCentralText(u"I");
    chord1->setScore(m_score);

    AlterationChord* chord2 = toAlterationChord(chord1->linkedClone());
    ASSERT_NE(chord2, nullptr);
    chord2->setScore(m_score);

    m_score->startCmd(TranslatableString::untranslatable("Edit linked slot"));
    chord1->undoSetSlotText(2, u"#11");
    m_score->endCmd();

    EXPECT_EQ(chord1->slotText(2), u"#11");
    EXPECT_EQ(chord2->slotText(2), u"#11");

    m_score->undoStack()->undo(nullptr);
    EXPECT_TRUE(chord1->slotText(2).isEmpty());
    EXPECT_TRUE(chord2->slotText(2).isEmpty());

    m_score->undoStack()->redo();
    EXPECT_EQ(chord1->slotText(2), u"#11");
    EXPECT_EQ(chord2->slotText(2), u"#11");

    delete chord1;
    delete chord2;
}

TEST_F(Engraving_AlterationChordTests, PreserveSlotOnlyAnnotationOnEndEdit)
{
    Measure* m = Factory::createMeasure(m_score);
    m_score->measures()->append(m);
    Segment* seg = m->getSegment(SegmentType::ChordRest, Fraction(0, 1));
    ASSERT_NE(seg, nullptr);

    m_score->startCmd(TranslatableString::untranslatable("Add chord"));
    AlterationChord* chord = m_score->addAlterationChord(seg);
    m_score->endCmd();
    ASSERT_NE(chord, nullptr);

    // Empty central text, non-empty slot
    chord->setCentralText(u"");
    chord->setSlotText(4, u"7#");

    EditData ed;
    chord->startEdit(ed);
    chord->endEdit(ed);

    // Annotation must NOT be removed
    const auto& annos = seg->annotations();
    EXPECT_TRUE(std::find(annos.begin(), annos.end(), chord) != annos.end());
    EXPECT_EQ(chord->slotText(4), u"7#");
}

TEST_F(Engraving_AlterationChordTests, LayoutRealisticWideGlyphsAndSpatium)
{
    Measure* m = Factory::createMeasure(m_score);
    m_score->measures()->append(m);
    Segment* seg = m->getSegment(SegmentType::ChordRest, Fraction(0, 1));
    ASSERT_NE(seg, nullptr);

    AlterationChord* chord = Factory::createAlterationChord(seg);
    ASSERT_NE(chord, nullptr);
    chord->setAutoplace(false);

    chord->setCentralText(u"IV");
    chord->setSlotText(1, u"6#");
    chord->setSlotText(2, u"#11");

    double sp = 15.0;
    m_score->style().setSpatium(sp);

    m_score->renderer()->layoutItem(static_cast<TextBase*>(chord));

    const AlterationChord::LayoutData* ldata = chord->ldata();
    ASSERT_NE(ldata, nullptr);

    // Realistic "6#" and "#11" on top edge must not overlap
    EXPECT_LE(ldata->slotRect[1].right(), ldata->slotRect[2].left() + 0.001);

    // Slot size must scale with spatium
    EXPECT_GE(ldata->slotSize, 0.65 * sp);

    // Bounding box must enclose both slot rects
    RectF bbox = ldata->bbox();
    EXPECT_TRUE(bbox.contains(ldata->slotRect[1].topLeft()));
    EXPECT_TRUE(bbox.contains(ldata->slotRect[1].bottomRight()));
    EXPECT_TRUE(bbox.contains(ldata->slotRect[2].topLeft()));
    EXPECT_TRUE(bbox.contains(ldata->slotRect[2].bottomRight()));

    delete chord;
}

TEST_F(Engraving_AlterationChordTests, SlotUsabilityFocusSelectAllPasteCutSurrogates)
{
    AlterationChord* chord = Factory::createAlterationChord(m_score->dummy());
    ASSERT_NE(chord, nullptr);

    chord->setSlotText(0, u"b9");
    chord->setSlotText(1, u"𝄫"); // musical flat double surrogate pair (U+1D12B)

    // Tab into slot 0 -> field should be selected
    EditData ed;
    ed.key = Key_Tab;
    chord->edit(ed);
    EXPECT_EQ(chord->activeSlot(), 0);
    EXPECT_TRUE(chord->isSlotSelected());

    // Typing '3' replaces 'b9' because whole field was selected
    ed.key = 0;
    ed.s = u"3";
    chord->edit(ed);
    EXPECT_EQ(chord->slotText(0), u"3");
    EXPECT_FALSE(chord->isSlotSelected());

    // Ctrl+A selects whole active slot field
    ed.key = Key_A;
    ed.modifiers = ControlModifier;
    chord->edit(ed);
    EXPECT_TRUE(chord->isSlotSelected());

    // Ctrl+Z bubbles up (isEditAllowed must return false)
    ed.key = Key_Z;
    ed.modifiers = ControlModifier;
    ed.s = u"\x1a";
    EXPECT_FALSE(chord->isEditAllowed(ed));

    // Paste into active slot
    chord->paste(u"6#");
    EXPECT_EQ(chord->slotText(0), u"6#");
    // Central text remains default "I"
    EXPECT_EQ(chord->centralText(), u"I");

    // Copy from active slot
    chord->selectAll();
    std::shared_ptr<TextEditData> ted = std::make_shared<TextEditData>(chord);
    ted->e = chord;
    ed.addData(ted);
    chord->editCopy(ed);
    EXPECT_EQ(ted->selectedPlainText, u"6#");

    // Cut / deleteSelectedText
    EXPECT_TRUE(chord->deleteSelectedText());
    EXPECT_TRUE(chord->slotText(0).isEmpty());

    // Tab to slot 1 with surrogate pair
    chord->nextSlot();
    EXPECT_EQ(chord->activeSlot(), 1);
    chord->setSlotSelected(false); // unselect so backspace tests surrogate pair preservation
    ed.modifiers = KeyboardModifier::NoModifier;
    ed.key = Key_Backspace;
    chord->edit(ed);
    EXPECT_TRUE(chord->slotText(1).isEmpty()); // surrogate pair removed completely, not split

    delete chord;
}

TEST_F(Engraving_AlterationChordTests, CentralTextXmlEscapingAndReplacement)
{
    Measure* m = Factory::createMeasure(m_score);
    m_score->measures()->append(m);
    Segment* seg = m->getSegment(SegmentType::ChordRest, Fraction(0, 1));
    ASSERT_NE(seg, nullptr);

    AlterationChord* chord = Factory::createAlterationChord(seg);
    ASSERT_NE(chord, nullptr);
    chord->setAutoplace(false);

    // Arbitrary text with XML special characters
    chord->setCentralText(u"<IV&V>");
    EXPECT_EQ(chord->centralText(), u"<IV&V>");
    EXPECT_TRUE(chord->xmlText().contains(u"&lt;"));
    EXPECT_TRUE(chord->xmlText().contains(u"&amp;"));

    // Start edit without move pos selects default text
    chord->setCentralText(u"I");
    EditData ed;
    chord->startEdit(ed);
    EXPECT_TRUE(chord->cursor()->hasSelection());

    // Typing replaces selected text
    ed.key = 0;
    ed.s = u"IV";
    chord->edit(ed);
    EXPECT_EQ(chord->centralText(), u"IV");

    delete chord;
}

TEST_F(Engraving_AlterationChordTests, FullScoreSaveAndReopen)
{
    Measure* m = Factory::createMeasure(m_score);
    m_score->measures()->append(m);
    Segment* seg = m->getSegment(SegmentType::ChordRest, Fraction(0, 1));
    ASSERT_NE(seg, nullptr);

    Rest* rest = Factory::createRest(m_score->dummy());
    rest->setTicks(Fraction(4, 4));
    rest->setTrack(0);
    seg->add(rest);

    AlterationChord* chord = m_score->addAlterationChord(seg);
    ASSERT_NE(chord, nullptr);
    chord->setCentralText(u"IV");
    chord->setSlotText(0, u"b9");
    chord->setSlotText(3, u"6#");
    chord->setSlotText(7, u"#11");

    String saveName = u"alterationChordFullScoreTest.mscx";
    ASSERT_TRUE(ScoreRW::saveScore(m_score, saveName));

    MasterScore* reloaded = ScoreRW::readScore(saveName, true);
    ASSERT_NE(reloaded, nullptr);

    Measure* rm = reloaded->firstMeasure();
    ASSERT_NE(rm, nullptr);
    AlterationChord* foundChord = nullptr;
    for (Segment& s : rm->segments()) {
        for (EngravingItem* anno : s.annotations()) {
            if (anno->isAlterationChord()) {
                foundChord = toAlterationChord(anno);
                break;
            }
        }
        if (foundChord) {
            break;
        }
    }

    ASSERT_NE(foundChord, nullptr);
    EXPECT_EQ(foundChord->centralText(), u"IV");
    EXPECT_EQ(foundChord->slotText(0), u"b9");
    EXPECT_EQ(foundChord->slotText(3), u"6#");
    EXPECT_EQ(foundChord->slotText(7), u"#11");

    delete reloaded;
}

TEST_F(Engraving_AlterationChordTests, PaletteSymbolsAndCommittedTextTargetSelectedSlot)
{
    Measure* measure = Factory::createMeasure(m_score);
    m_score->measures()->append(measure);
    Segment* segment = measure->getSegment(SegmentType::ChordRest, Fraction(0, 1));
    auto chord = Factory::createAlterationChord(segment);
    chord->setTrack(0);
    chord->setAutoplace(false);
    EditData ed;
    chord->startEdit(ed);
    chord->setActiveSlot(4);
    chord->setSlotText(4, u"old");
    chord->setSlotSelected(true);
    const String sharp = m_score->engravingFont()->toString(SymId::accidentalSharp);
    ASSERT_FALSE(sharp.isEmpty());

    m_score->startCmd(TranslatableString::untranslatable("Insert symbol"));
    auto symbol = Factory::createSymbol(m_score->dummy());
    symbol->setSym(SymId::accidentalSharp);
    ed.dropElement = symbol;
    EXPECT_TRUE(chord->acceptDrop(ed));
    chord->drop(m_score->transactionManager()->currentOrDummyTransaction(), ed);
    m_score->endCmd();
    EXPECT_EQ(chord->slotText(4), sharp);
    m_score->undoStack()->undo(nullptr);
    EXPECT_EQ(chord->slotText(4), u"old");
    m_score->undoStack()->redo();
    EXPECT_EQ(chord->slotText(4), sharp);
    EXPECT_EQ(chord->centralText(), u"I");
    EXPECT_FALSE(chord->isSlotSelected());
    EXPECT_EQ(chord->activeSlot(), 4);

    TextBase* text = chord;
    m_score->startCmd(TranslatableString::untranslatable("Append text"));
    text->insertText(u"6");
    m_score->endCmd();
    EXPECT_EQ(chord->slotText(4), sharp + u"6");
    m_score->undoStack()->undo(nullptr);
    EXPECT_EQ(chord->slotText(4), sharp);
    m_score->undoStack()->redo();
    EXPECT_EQ(chord->slotText(4), sharp + u"6");

    chord->setSlotSelected(true);
    m_score->startCmd(TranslatableString::untranslatable("Insert Unicode symbol"));
    auto unicode = Factory::createFSymbol(m_score->dummy());
    unicode->setCode(0x266D);
    ed.dropElement = unicode;
    EXPECT_TRUE(chord->acceptDrop(ed));
    chord->drop(m_score->transactionManager()->currentOrDummyTransaction(), ed);
    m_score->endCmd();
    EXPECT_EQ(chord->slotText(4), u"♭");
    EXPECT_EQ(chord->centralText(), u"I");

    chord->setSlotText(5, sharp + u"6");
    chord->endEdit(ed);
    auto restored = toAlterationChord(ScoreRW::writeReadElement(chord));
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->slotText(4), u"♭");
    EXPECT_EQ(restored->slotText(5), sharp + u"6");
    delete restored;
    delete chord;
}

TEST_F(Engraving_AlterationChordTests, CompactSideLabelsKeepSelectableDisjointSlots)
{
    Measure* measure = Factory::createMeasure(m_score);
    m_score->measures()->append(measure);
    Segment* segment = measure->getSegment(SegmentType::ChordRest, Fraction(0, 1));
    auto chord = Factory::createAlterationChord(segment);
    chord->setTrack(0);
    chord->setAutoplace(false);
    chord->setCentralText(u"V");
    chord->setAutoplace(false);
    chord->setSlotText(4, u"6");
    chord->setSlotText(5, u"5");
    m_score->renderer()->layoutItem(static_cast<TextBase*>(chord));
    const auto* ldata = chord->ldata();
    ASSERT_FALSE(ldata->slotTextRuns[4].empty());
    const auto& run = ldata->slotTextRuns[4].front();
    const RectF label = muse::draw::FontMetrics(run.font).boundingRect(run.text).translated(run.pos);
    const RectF central = ldata->blocks.front().boundingRect();
    EXPECT_NEAR(label.left() - central.right(), 0.05 * chord->spatium(), 0.001);
    EditData ed;
    chord->startEdit(ed);
    chord->setPos(10.0, 20.0);
    for (int i = 0; i < AlterationChord::SLOTS_COUNT; ++i) {
        ed.startMove = chord->canvasPos() + ldata->slotRect[i].center();
        EXPECT_TRUE(chord->mousePress(ed));
        EXPECT_EQ(chord->activeSlot(), i);
        for (int j = i + 1; j < AlterationChord::SLOTS_COUNT; ++j) {
            const RectF overlap = ldata->slotRect[i].intersected(ldata->slotRect[j]);
            EXPECT_LE(overlap.width() * overlap.height(), 0.001);
        }
    }
    chord->endEdit(ed);
    delete chord;
}

TEST_F(Engraving_AlterationChordTests, MultiCharacterSlotInputWithAltGrAndShift)
{
    auto chord = Factory::createAlterationChord(m_score->dummy());
    chord->setActiveSlot(4);
    chord->setSlotSelected(true);
    EditData ed;
    ed.key = Key_3;
    ed.s = u"3";
    ASSERT_TRUE(chord->isEditAllowed(ed));
    ASSERT_TRUE(chord->edit(ed));
    EXPECT_EQ(chord->slotText(4), u"3");

    // AltGr+X produces # on some Windows keyboard layouts, including Hungarian.
    // Its key code must not be mistaken for the Ctrl+X cut shortcut.
    ed.key = Key_X;
    ed.modifiers = ControlModifier | AltModifier;
    ed.s = u"#";
    ASSERT_TRUE(chord->isEditAllowed(ed));
    ASSERT_TRUE(chord->edit(ed));
    EXPECT_EQ(chord->slotText(4), u"3#");

    ed.modifiers = NoModifier;
    ed.key = Key_B;
    ed.s = u"b";
    ASSERT_TRUE(chord->edit(ed));
    EXPECT_EQ(chord->slotText(4), u"3#b");
    ed.modifiers = ShiftModifier;
    ed.s = u"B";
    ASSERT_TRUE(chord->edit(ed));
    EXPECT_EQ(chord->slotText(4), u"3#bB");

    ed.modifiers = ControlModifier;
    ed.s = u"x";
    ed.key = Key_X;
    EXPECT_FALSE(chord->isEditAllowed(ed));
    ed.modifiers = ControlModifier | AltModifier;
    ed.s = String(Char(0x18));
    EXPECT_FALSE(chord->isEditAllowed(ed));
    EXPECT_EQ(chord->centralText(), u"I");
    delete chord;
}

TEST_F(Engraving_AlterationChordTests, PaletteModifiersAppendToNumberAndInsertIntoCentralText)
{
    Measure* measure = Factory::createMeasure(m_score);
    m_score->measures()->append(measure);
    Segment* segment = measure->getSegment(SegmentType::ChordRest, Fraction(0, 1));
    auto chord = Factory::createAlterationChord(segment);
    chord->setTrack(0);
    chord->setAutoplace(false);
    EditData ed;
    chord->startEdit(ed);
    chord->setActiveSlot(4);
    chord->setSlotText(4, u"3");
    // The toolbar clears the focus-time whole-field selection before dropping.
    chord->setSlotSelected(false);
    const SymId symbols[] = { SymId::accidentalSharp, SymId::accidentalFlat, SymId::articTenutoAbove };
    String expected = u"3";
    for (SymId id : symbols) {
        m_score->startCmd(TranslatableString::untranslatable("Append toolbar symbol"));
        auto symbol = Factory::createSymbol(m_score->dummy());
        symbol->setSym(id);
        ed.dropElement = symbol;
        EXPECT_TRUE(chord->acceptDrop(ed));
        chord->drop(m_score->transactionManager()->currentOrDummyTransaction(), ed);
        m_score->endCmd();
        expected += m_score->engravingFont()->toString(id);
        EXPECT_EQ(chord->slotText(4), expected);
        EXPECT_EQ(chord->centralText(), u"I");
    }
    m_score->undoStack()->undo(nullptr);
    EXPECT_EQ(chord->slotText(4), u"3" + m_score->engravingFont()->toString(symbols[0])
              + m_score->engravingFont()->toString(symbols[1]));
    m_score->undoStack()->redo();
    EXPECT_EQ(chord->slotText(4), expected);

    chord->setActiveSlot(AlterationChord::FOCUS_CENTRAL);
    chord->selectAll();
    m_score->startCmd(TranslatableString::untranslatable("Central toolbar symbol"));
    auto symbol = Factory::createSymbol(m_score->dummy());
    symbol->setSym(SymId::accidentalFlat);
    ed.dropElement = symbol;
    chord->drop(m_score->transactionManager()->currentOrDummyTransaction(), ed);
    m_score->endCmd();
    EXPECT_EQ(chord->centralText(), m_score->engravingFont()->toString(SymId::accidentalFlat));
    EXPECT_EQ(chord->slotText(4), expected);
    chord->endEdit(ed);
    auto restored = toAlterationChord(ScoreRW::writeReadElement(chord));
    EXPECT_EQ(restored->slotText(4), expected);
    EXPECT_EQ(restored->centralText(), chord->centralText());
    delete restored;
    delete chord;
}
