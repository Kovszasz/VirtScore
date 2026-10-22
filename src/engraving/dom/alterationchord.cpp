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

#include "alterationchord.h"
#include "segment.h"
#include "score.h"
#include "symbol.h"
#include "engraving/iengravingfont.h"
#include "../editing/editdata.h"
#include "../editing/textedit.h"
#include "../editing/transaction/undoablecommand.h"
#include "../rendering/iscorerenderer.h"
#include "draw/fontmetrics.h"
#include "draw/types/transform.h"

namespace mu::engraving {
//---------------------------------------------------------
//   ChangeAlterationSlotText
//---------------------------------------------------------

class ChangeAlterationSlotText : public UndoableCommand
{
    OBJECT_ALLOCATOR(engraving, ChangeAlterationSlotText)

public:
    UNDO_NAME("ChangeAlterationSlotText")
    UNDO_CHANGED_OBJECTS({ m_chord })

    ChangeAlterationSlotText(AlterationChord* chord, int slot, const String& newText)
        : m_chord(chord), m_slot(slot), m_oldText(chord->slotText(slot)), m_newText(newText)
    {
    }

    void undo() override
    {
        m_chord->setSlotText(m_slot, m_oldText);
        m_chord->triggerLayout();
    }

    void redo() override
    {
        m_chord->setSlotText(m_slot, m_newText);
        m_chord->triggerLayout();
    }

private:
    AlterationChord* m_chord = nullptr;
    int m_slot = 0;
    String m_oldText;
    String m_newText;
};

//---------------------------------------------------------
//   AlterationChord
//---------------------------------------------------------

static const ElementStyle ALTERATION_CHORD_STYLE = {
    StyledProperty{ Sid::staffTextPlacement, Pid::PLACEMENT },
    StyledProperty{ Sid::staffTextMinDistance, Pid::MIN_DISTANCE },
};

AlterationChord::AlterationChord(DummyParentOr<Segment> parent, TextStyleType tid)
    : TextBase(ElementType::ALTERATION_CHORD, parent, tid, ElementFlag::MOVABLE | ElementFlag::ON_STAFF)
{
    initElementStyle(&ALTERATION_CHORD_STYLE);
    setPlainText(u"I");
    for (int i = 0; i < SLOTS_COUNT; ++i) {
        m_slots[i] = String();
    }
}

AlterationChord::AlterationChord(const AlterationChord& other)
    : TextBase(other)
    , m_slots(other.m_slots)
    , m_activeSlot(other.m_activeSlot)
    , m_slotSelected(other.m_slotSelected)
{
}

EngravingItem* AlterationChord::linkedClone()
{
    AlterationChord* item = clone();
    item->linkTo(this);
    return item;
}

Segment* AlterationChord::segment() const
{
    return toSegment(ownershipParent());
}

void AlterationChord::setCentralText(const String& s)
{
    setPlainText(s);
    triggerLayout();
}

String AlterationChord::slotText(int slot) const
{
    if (slot >= 0 && slot < SLOTS_COUNT) {
        return m_slots[slot];
    }
    return String();
}

void AlterationChord::setSlotText(int slot, const String& text)
{
    if (slot >= 0 && slot < SLOTS_COUNT) {
        m_slots[slot] = text;
        triggerLayout();
    }
}

void AlterationChord::undoSetSlotText(int slot, const String& text)
{
    if (slot < 0 || slot >= SLOTS_COUNT || m_slots[slot] == text) {
        return;
    }
    const std::list<EngravingObject*> linkedObjects = linkListForPropertyPropagation();
    for (EngravingObject* obj : linkedObjects) {
        AlterationChord* ac = toAlterationChord(obj);
        if (ac) {
            ac->score()->undo(new ChangeAlterationSlotText(ac, slot, text));
        }
    }
}

void AlterationChord::setSlots(const std::array<String, SLOTS_COUNT>& newSlots)
{
    m_slots = newSlots;
    triggerLayout();
}

void AlterationChord::nextSlot()
{
    if (m_activeSlot == FOCUS_CENTRAL) {
        m_activeSlot = 0;
    } else if (m_activeSlot == SLOTS_COUNT - 1) {
        m_activeSlot = FOCUS_CENTRAL;
    } else {
        m_activeSlot++;
    }
}

void AlterationChord::prevSlot()
{
    if (m_activeSlot == FOCUS_CENTRAL) {
        m_activeSlot = SLOTS_COUNT - 1;
    } else if (m_activeSlot == 0) {
        m_activeSlot = FOCUS_CENTRAL;
    } else {
        m_activeSlot--;
    }
}

void AlterationChord::added()
{
    TextBase::added();
}

void AlterationChord::removed()
{
    TextBase::removed();
}

void AlterationChord::startEdit(EditData& ed)
{
    TextBase::startEdit(ed);

    const LayoutData* l = ldata();
    if (l && !ed.startMove.isNull()) {
        PointF localPos = ed.startMove - canvasPos();
        if (!muse::RealIsNull(textAngle())) {
            muse::draw::Transform t;
            t.rotate(-textAngle());
            localPos = t.map(localPos);
        }
        for (int i = 0; i < SLOTS_COUNT; ++i) {
            if (l->slotRect[i].contains(localPos)) {
                m_activeSlot = i;
                m_slotSelected = true;
                return;
            }
        }
    }
    m_activeSlot = FOCUS_CENTRAL;
    m_slotSelected = false;
    if (ed.startMove.isNull()) {
        selectAll();
    }
}

void AlterationChord::endEdit(EditData& ed)
{
    TextBase::endEdit(ed);
    m_activeSlot = FOCUS_CENTRAL;
    m_slotSelected = false;
    triggerLayout();
}

bool AlterationChord::mousePress(EditData& ed)
{
    const LayoutData* l = ldata();
    if (l && !ed.startMove.isNull()) {
        PointF localPos = ed.startMove - canvasPos();
        if (!muse::RealIsNull(textAngle())) {
            muse::draw::Transform t;
            t.rotate(-textAngle());
            localPos = t.map(localPos);
        }
        for (int i = 0; i < SLOTS_COUNT; ++i) {
            if (l->slotRect[i].contains(localPos)) {
                m_activeSlot = i;
                m_slotSelected = true;
                score()->setUpdateAll();
                return true;
            }
        }
        if (l->centralRect.contains(localPos)) {
            m_activeSlot = FOCUS_CENTRAL;
            m_slotSelected = false;
            return TextBase::mousePress(ed);
        }
    }

    m_activeSlot = FOCUS_CENTRAL;
    m_slotSelected = false;
    return TextBase::mousePress(ed);
}

bool AlterationChord::isEditAllowed(EditData& ed) const
{
    if (ed.key == Key_Tab || ed.key == Key_Backtab) {
        return true;
    }

    if (m_activeSlot >= 0 && m_activeSlot < SLOTS_COUNT) {
        bool ctrlPressed = ed.modifiers & ControlModifier;
        bool shiftPressed = ed.modifiers & ShiftModifier;

        // Normal Ctrl/Cmd+Z, Ctrl+Y, Ctrl+C, Ctrl+X, Ctrl+V must bubble to application level
        if (ctrlPressed && !shiftPressed) {
            if (ed.key == Key_Z || ed.key == Key_Y || ed.key == Key_C || ed.key == Key_X || ed.key == Key_V) {
                return false;
            }
            if (ed.key == Key_A) {
                return true;
            }
        }

        if (ed.key == Key_Backspace || ed.key == Key_Delete
            || ed.key == Key_Escape || ed.key == Key_Return || ed.key == Key_Enter) {
            return true;
        }

        if (!ed.s.isEmpty() && !ctrlPressed) {
            return true;
        }

        return false;
    }

    return TextBase::isEditAllowed(ed);
}

bool AlterationChord::edit(EditData& ed)
{
    if (ed.key == Key_Tab || ed.key == Key_Backtab) {
        if (ed.key == Key_Backtab || (ed.modifiers & ShiftModifier)) {
            prevSlot();
        } else {
            nextSlot();
        }
        if (m_activeSlot >= 0 && m_activeSlot < SLOTS_COUNT) {
            m_slotSelected = true;
        } else {
            m_slotSelected = false;
            selectAll();
        }
        score()->setUpdateAll();
        return true;
    }

    if (m_activeSlot >= 0 && m_activeSlot < SLOTS_COUNT) {
        bool ctrlPressed = ed.modifiers & ControlModifier;
        bool shiftPressed = ed.modifiers & ShiftModifier;

        if (ctrlPressed && !shiftPressed && ed.key == Key_A) {
            m_slotSelected = true;
            score()->setUpdateAll();
            return true;
        }

        String current = m_slots[m_activeSlot];

        if (ed.key == Key_Backspace) {
            if (m_slotSelected) {
                undoSetSlotText(m_activeSlot, String());
                m_slotSelected = false;
            } else if (!current.isEmpty()) {
                size_t len = current.size();
                if (len >= 2 && current.at(len - 1).isLowSurrogate() && current.at(len - 2).isHighSurrogate()) {
                    undoSetSlotText(m_activeSlot, current.left(len - 2));
                } else {
                    undoSetSlotText(m_activeSlot, current.left(len - 1));
                }
            }
            score()->setUpdateAll();
            return true;
        }

        if (ed.key == Key_Delete) {
            if (m_slotSelected || !current.isEmpty()) {
                undoSetSlotText(m_activeSlot, String());
                m_slotSelected = false;
            }
            score()->setUpdateAll();
            return true;
        }

        if (!ed.s.isEmpty() && !ctrlPressed && ed.key != Key_Escape && ed.key != Key_Return && ed.key != Key_Enter) {
            if (m_slotSelected) {
                undoSetSlotText(m_activeSlot, ed.s);
                m_slotSelected = false;
            } else {
                undoSetSlotText(m_activeSlot, current + ed.s);
            }
            score()->setUpdateAll();
            return true;
        }

        return false;
    }

    return TextBase::edit(ed);
}

void AlterationChord::selectAll()
{
    if (m_activeSlot >= 0 && m_activeSlot < SLOTS_COUNT) {
        m_slotSelected = true;
        score()->setUpdateAll();
        return;
    }
    TextBase::selectAll();
}

bool AlterationChord::deleteSelectedText()
{
    if (m_activeSlot >= 0 && m_activeSlot < SLOTS_COUNT) {
        if (m_slotSelected) {
            undoSetSlotText(m_activeSlot, String());
            m_slotSelected = false;
            score()->setUpdateAll();
            return true;
        }
        return false;
    }
    return TextBase::deleteSelectedText();
}

void AlterationChord::editCopy(EditData& ed)
{
    if (m_activeSlot >= 0 && m_activeSlot < SLOTS_COUNT) {
        TextEditData* ted = static_cast<TextEditData*>(ed.getData(this).get());
        if (ted) {
            String text = slotText(m_activeSlot);
            ted->selectedText = text.toXmlEscaped();
            ted->selectedPlainText = text;
        }
        return;
    }
    TextBase::editCopy(ed);
}

void AlterationChord::paste(const String& txt)
{
    if (m_activeSlot >= 0 && m_activeSlot < SLOTS_COUNT) {
        score()->startCmd(TranslatableString("undoableAction", "Paste text"));
        if (m_slotSelected) {
            undoSetSlotText(m_activeSlot, txt);
            m_slotSelected = false;
        } else {
            undoSetSlotText(m_activeSlot, slotText(m_activeSlot) + txt);
        }
        score()->endCmd();
        score()->setUpdateAll();
        return;
    }
    TextBase::paste(txt);
}

void AlterationChord::insertText(const String& text)
{
    if (m_activeSlot < 0 || m_activeSlot >= SLOTS_COUNT) {
        TextBase::insertText(text);
        return;
    }
    undoSetSlotText(m_activeSlot, m_slotSelected ? text : slotText(m_activeSlot) + text);
    m_slotSelected = false;
    score()->setUpdateAll();
}

void AlterationChord::insertSym(SymId id)
{
    if (m_activeSlot < 0 || m_activeSlot >= SLOTS_COUNT) {
        TextBase::insertSym(id);
        return;
    }
    insertText(score()->engravingFont()->toString(id));
}

EngravingItem* AlterationChord::drop(Transaction& tx, EditData& ed)
{
    if (m_activeSlot >= 0 && m_activeSlot < SLOTS_COUNT) {
        EngravingItem* element = ed.dropElement;
        if (element->isSymbol()) {
            insertSym(toSymbol(element)->sym());
        } else if (element->isFSymbol()) {
            insertText(toFSymbol(element)->toString());
        } else {
            return TextBase::drop(tx, ed);
        }
        delete element;
        return nullptr;
    }
    return TextBase::drop(tx, ed);
}

void AlterationChord::calculateSlotsLayout(LayoutData* ldata, double sp, const AlterationChord* item)
{
    RectF cbb = ldata->shape().bbox();
    double slotSize = 0.65 * sp;
    const double padding = 0.05 * sp;
    std::array<RectF, SLOTS_COUNT> textBounds;

    for (int i = 0; i < SLOTS_COUNT; ++i) {
        auto& runs = ldata->slotTextRuns[i];
        runs.clear();
        if (!item) {
            continue;
        }
        muse::draw::Font textFont = item->font();
        textFont.setPointSizeF(textFont.pointSizeF() * 0.7);
        muse::draw::Font symbolFont = textFont;
        symbolFont.setFamily(item->style().styleSt(Sid::musicalTextFont), muse::draw::Font::Type::MusicSymbolText);
        // SMuFL characters inserted by the symbol palette occupy the private-use area.
        // Keep ordinary letters/digits in the user's font, including in mixed fields.
        const String text = item->slotText(i);
        for (size_t j = 0; j < text.size(); ++j) {
            const Char c = text.at(j);
            const bool symbol = c.unicode() >= 0xE000 && c.unicode() <= 0xF8FF;
            const auto& font = symbol ? symbolFont : textFont;
            if (runs.empty() || runs.back().font != font) {
                runs.push_back({ String(), font, PointF() });
            }
            runs.back().text += c;
        }
        double x = 0.0;
        for (auto& run : runs) {
            muse::draw::FontMetrics fm(run.font);
            run.pos = PointF(x, 0.0);
            const RectF bounds = fm.boundingRect(run.text).translated(run.pos);
            textBounds[i].unite(bounds);
            x += fm.horizontalAdvance(run.text);
        }
        slotSize = std::max(slotSize, std::max(textBounds[i].width(), textBounds[i].height()) + padding);
    }

    ldata->slotSize = slotSize;

    double centralW = std::max(0.1 * sp, cbb.width() + 2.0 * padding);
    double centralH = std::max(2.0 * slotSize, cbb.height() + 2.0 * padding);

    double cx = cbb.center().x();
    double cy = cbb.center().y();
    RectF centralRect(cx - centralW / 2.0, cy - centralH / 2.0, centralW, centralH);
    ldata->centralRect = centralRect;

    // The two top/bottom fields may extend beyond a narrow central glyph. Move
    // only the corners outward so all twelve hit targets stay disjoint.
    const double leftCorner = std::min(centralRect.left(), cx - slotSize) - slotSize;
    const double rightCorner = std::max(centralRect.right(), cx + slotSize);

    // Top row: 0=TopLeft, 1=Top1, 2=Top2, 3=TopRight (all square slotSize x slotSize)
    ldata->slotRect[0] = RectF(leftCorner, centralRect.top() - slotSize, slotSize, slotSize);
    ldata->slotRect[1] = RectF(centralRect.center().x() - slotSize, centralRect.top() - slotSize, slotSize, slotSize);
    ldata->slotRect[2] = RectF(centralRect.center().x(), centralRect.top() - slotSize, slotSize, slotSize);
    ldata->slotRect[3] = RectF(rightCorner, centralRect.top() - slotSize, slotSize, slotSize);

    // Right column: 4=Right1, 5=Right2
    ldata->slotRect[4] = RectF(centralRect.right(), centralRect.center().y() - slotSize, slotSize, slotSize);
    ldata->slotRect[5] = RectF(centralRect.right(), centralRect.center().y(), slotSize, slotSize);

    // Bottom row: 6=BottomRight, 7=Bottom2, 8=Bottom1, 9=BottomLeft
    ldata->slotRect[6] = RectF(rightCorner, centralRect.bottom(), slotSize, slotSize);
    ldata->slotRect[7] = RectF(centralRect.center().x(), centralRect.bottom(), slotSize, slotSize);
    ldata->slotRect[8] = RectF(centralRect.center().x() - slotSize, centralRect.bottom(), slotSize, slotSize);
    ldata->slotRect[9] = RectF(leftCorner, centralRect.bottom(), slotSize, slotSize);

    // Left column: 10=Left2, 11=Left1
    ldata->slotRect[10] = RectF(centralRect.left() - slotSize, centralRect.center().y(), slotSize, slotSize);
    ldata->slotRect[11] = RectF(centralRect.left() - slotSize, centralRect.center().y() - slotSize, slotSize, slotSize);

    for (int i = 0; i < SLOTS_COUNT; ++i) {
        const RectF& rect = ldata->slotRect[i];
        PointF origin = rect.center() - textBounds[i].center();
        // Side labels hug the central glyph instead of centering in a wide square.
        if (i == 4 || i == 5) {
            origin.setX(rect.left() - textBounds[i].left());
        } else if (i == 10 || i == 11) {
            origin.setX(rect.right() - textBounds[i].right());
        }
        for (auto& run : ldata->slotTextRuns[i]) {
            run.pos += origin;
        }
    }
}
} // namespace mu::engraving
