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
    m_slotCursor = -1;
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
    m_slotCursor = -1;
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
                focusSlotAt(i, localPos.x());
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
                focusSlotAt(i, localPos.x());
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
        const bool altGrText = ctrlPressed && (ed.modifiers & AltModifier) && !ed.s.isEmpty()
                               && ed.s.front().unicode() >= 0x20 && ed.s.front().unicode() != 0x7F;

        // Normal Ctrl/Cmd+Z, Ctrl+Y, Ctrl+C, Ctrl+X, Ctrl+V must bubble to application level
        if (ctrlPressed && !shiftPressed && !altGrText) {
            if (ed.key == Key_Z || ed.key == Key_Y || ed.key == Key_C || ed.key == Key_X || ed.key == Key_V) {
                return false;
            }
            if (ed.key == Key_A) {
                return true;
            }
        }

        if (ed.key == Key_Backspace || ed.key == Key_Delete
            || ed.key == Key_Left || ed.key == Key_Right || ed.key == Key_Home || ed.key == Key_End
            || ed.key == Key_Escape || ed.key == Key_Return || ed.key == Key_Enter) {
            return true;
        }

        if (!ed.s.isEmpty() && (!ctrlPressed || altGrText)) {
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
        m_slotSelected = false;
        if (m_activeSlot == FOCUS_CENTRAL) {
            selectAll();
        }
        score()->setUpdateAll();
        return true;
    }

    if (m_activeSlot >= 0 && m_activeSlot < SLOTS_COUNT) {
        bool ctrlPressed = ed.modifiers & ControlModifier;
        bool shiftPressed = ed.modifiers & ShiftModifier;
        const bool altGrText = ctrlPressed && (ed.modifiers & AltModifier) && !ed.s.isEmpty()
                               && ed.s.front().unicode() >= 0x20 && ed.s.front().unicode() != 0x7F;

        if (ctrlPressed && !shiftPressed && !altGrText && ed.key == Key_A) {
            m_slotSelected = true;
            score()->setUpdateAll();
            return true;
        }

        String current = m_slots[m_activeSlot];

        size_t pos = slotCursorPosition();
        auto previous = [&current](size_t p) {
            if (p == 0) {
                return p;
            }
            --p;
            if (p > 0 && current.at(p).isLowSurrogate() && current.at(p - 1).isHighSurrogate()) {
                --p;
            }
            return p;
        };
        auto next = [&current](size_t p) {
            if (p == current.size()) {
                return p;
            }
            if (current.at(p).isHighSurrogate() && p + 1 < current.size() && current.at(p + 1).isLowSurrogate()) {
                ++p;
            }
            return p + 1;
        };
        if (ed.key == Key_Left || ed.key == Key_Right || ed.key == Key_Home || ed.key == Key_End) {
            if (ed.key == Key_Home || (m_slotSelected && ed.key == Key_Left)) {
                pos = 0;
            } else if (ed.key == Key_End || (m_slotSelected && ed.key == Key_Right)) {
                pos = current.size();
            } else {
                pos = ed.key == Key_Left ? previous(pos) : next(pos);
            }
            m_slotCursor = static_cast<int>(pos);
            m_slotSelected = false;
            score()->setUpdateAll();
            return true;
        }

        if (ed.key == Key_Backspace || ed.key == Key_Delete) {
            if (m_slotSelected) {
                undoSetSlotText(m_activeSlot, String());
                m_slotCursor = 0;
            } else {
                const size_t start = ed.key == Key_Backspace ? previous(pos) : pos;
                const size_t end = ed.key == Key_Delete ? next(pos) : pos;
                undoSetSlotText(m_activeSlot, current.left(start) + current.mid(end));
                m_slotCursor = static_cast<int>(start);
            }
            m_slotSelected = false;
            score()->setUpdateAll();
            return true;
        }

        if (!ed.s.isEmpty() && (!ctrlPressed || altGrText) && ed.key != Key_Escape && ed.key != Key_Return && ed.key != Key_Enter) {
            insertText(ed.s);
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
            m_slotCursor = 0;
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
        insertText(txt);
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
    const size_t pos = m_slotSelected ? 0 : slotCursorPosition();
    const String current = slotText(m_activeSlot);
    undoSetSlotText(m_activeSlot, m_slotSelected ? text : current.left(pos) + text + current.mid(pos));
    m_slotCursor = static_cast<int>(pos + text.size());
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

size_t AlterationChord::slotCursorPosition() const
{
    const size_t length = slotText(m_activeSlot).size();
    return m_slotCursor < 0 ? length : std::min(static_cast<size_t>(m_slotCursor), length);
}

RectF AlterationChord::slotCursorRect() const
{
    if (m_activeSlot < 0 || m_activeSlot >= SLOTS_COUNT) {
        return RectF();
    }
    const auto* l = ldata();
    const RectF& rect = l->slotRect[m_activeSlot];
    double x = rect.left();
    size_t remaining = slotCursorPosition();
    for (const auto& run : l->slotTextRuns[m_activeSlot]) {
        const size_t count = std::min(remaining, run.text.size());
        x = run.pos.x() + muse::draw::FontMetrics(run.font).horizontalAdvance(run.text.left(count));
        if (remaining <= run.text.size()) {
            break;
        }
        remaining -= run.text.size();
    }
    return RectF(std::clamp(x, rect.left(), rect.right()), rect.top(), 0.025 * spatium(), rect.height());
}

void AlterationChord::focusSlotAt(int slot, double x)
{
    setActiveSlot(slot);
    m_slotSelected = false;
    double distance = 1e20;
    int closest = 0;
    const String text = slotText(slot);
    for (size_t p = 0; p <= text.size(); ++p) {
        if (p < text.size() && text.at(p).isLowSurrogate()) {
            continue;
        }
        m_slotCursor = static_cast<int>(p);
        const double d = std::abs(slotCursorRect().left() - x);
        if (d < distance) {
            distance = d;
            closest = static_cast<int>(p);
        }
    }
    m_slotCursor = closest;
}

void AlterationChord::calculateSlotsLayout(LayoutData* ldata, double sp, const AlterationChord* item)
{
    RectF cbb = ldata->shape().bbox();
    if (item) {
        RectF ink;
        for (const auto& block : ldata->blocks) {
            for (const auto& fragment : block.fragments()) {
                ink.unite(muse::draw::FontMetrics(fragment.font(item)).tightBoundingRect(fragment.text)
                          .translated(fragment.pos + PointF(0.0, block.y())));
            }
        }
        if (!ink.isEmpty()) {
            cbb = ink;
        }
    }
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
            const RectF bounds = fm.tightBoundingRect(run.text).translated(run.pos);
            textBounds[i].unite(bounds);
            x += fm.horizontalAdvance(run.text);
        }
        slotSize = std::max(slotSize, std::max(textBounds[i].width(), textBounds[i].height()) + padding);
    }

    ldata->slotSize = slotSize;
    double rowHeight = 0.65 * sp;
    double columnWidth = 0.65 * sp;
    for (int i : { 0, 1, 2, 3, 6, 7, 8, 9 }) {
        rowHeight = std::max(rowHeight, textBounds[i].height() + padding);
    }
    for (int i : { 0, 3, 4, 5, 6, 9, 10, 11 }) {
        columnWidth = std::max(columnWidth, textBounds[i].width() + padding);
    }

    double topWidth = 0.325 * sp;
    for (int i : { 1, 2, 7, 8 }) {
        topWidth = std::max(topWidth, textBounds[i].width() + padding);
    }
    double centralW = std::max(2.0 * topWidth, cbb.width() + 2.0 * padding);
    double sideHeight = 0.325 * sp;
    for (int i : { 4, 5, 10, 11 }) {
        sideHeight = std::max(sideHeight, textBounds[i].height() + padding);
    }
    double centralH = std::max(2.0 * sideHeight, cbb.height() + 2.0 * padding);

    double cx = cbb.center().x();
    double cy = cbb.center().y();
    RectF centralRect(cx - centralW / 2.0, cy - centralH / 2.0, centralW, centralH);
    ldata->centralRect = centralRect;

    // Corners share columns with the side fields. Top/bottom fields split the
    // central width, so wide side text cannot push corners out of alignment.
    const double leftCorner = centralRect.left() - columnWidth;
    const double rightCorner = centralRect.right();

    // Top row: 0=TopLeft, 1=Top1, 2=Top2, 3=TopRight
    ldata->slotRect[0] = RectF(leftCorner, centralRect.top() - rowHeight, columnWidth, rowHeight);
    ldata->slotRect[1] = RectF(centralRect.left(), centralRect.top() - rowHeight, centralW / 2.0, rowHeight);
    ldata->slotRect[2] = RectF(cx, centralRect.top() - rowHeight, centralW / 2.0, rowHeight);
    ldata->slotRect[3] = RectF(rightCorner, centralRect.top() - rowHeight, columnWidth, rowHeight);

    // Right column: 4=Right1, 5=Right2
    ldata->slotRect[4] = RectF(centralRect.right(), centralRect.top(), columnWidth, centralH / 2.0);
    ldata->slotRect[5] = RectF(centralRect.right(), centralRect.center().y(), columnWidth, centralH / 2.0);

    // Bottom row: 6=BottomRight, 7=Bottom2, 8=Bottom1, 9=BottomLeft
    ldata->slotRect[6] = RectF(rightCorner, centralRect.bottom(), columnWidth, rowHeight);
    ldata->slotRect[7] = RectF(cx, centralRect.bottom(), centralW / 2.0, rowHeight);
    ldata->slotRect[8] = RectF(centralRect.left(), centralRect.bottom(), centralW / 2.0, rowHeight);
    ldata->slotRect[9] = RectF(leftCorner, centralRect.bottom(), columnWidth, rowHeight);

    // Left column: 10=Left2, 11=Left1
    ldata->slotRect[10] = RectF(centralRect.left() - columnWidth, centralRect.center().y(), columnWidth, centralH / 2.0);
    ldata->slotRect[11] = RectF(centralRect.left() - columnWidth, centralRect.top(), columnWidth, centralH / 2.0);

    for (int i = 0; i < SLOTS_COUNT; ++i) {
        const RectF& rect = ldata->slotRect[i];
        PointF origin = rect.center() - textBounds[i].center();
        // Side labels hug the central glyph instead of centering in a wide square.
        if (i == 3 || i == 4 || i == 5 || i == 6) {
            origin.setX(rect.left() - textBounds[i].left());
        } else if (i == 0 || i == 9 || i == 10 || i == 11) {
            origin.setX(rect.right() - textBounds[i].right());
        }
        // Top and bottom rows, including corners, share their inward ink edge.
        if (i >= 0 && i <= 3) {
            origin.setY(rect.bottom() - textBounds[i].bottom());
        } else if (i >= 6 && i <= 9) {
            origin.setY(rect.top() - textBounds[i].top());
        }
        for (auto& run : ldata->slotTextRuns[i]) {
            run.pos += origin;
        }
    }
}
} // namespace mu::engraving
