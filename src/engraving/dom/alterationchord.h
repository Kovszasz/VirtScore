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

#ifndef MU_ENGRAVING_ALTERATIONCHORD_H
#define MU_ENGRAVING_ALTERATIONCHORD_H

#include <array>
#include <vector>
#include "textbase.h"

namespace mu::engraving {
class Segment;

enum class AlterationSlot : int {
    TopLeft = 0,
    Top = 1,
    TopRight = 2,
    Right1 = 3,
    Right2 = 4,
    BottomRight = 5,
    Bottom = 6,
    BottomLeft = 7,
    Left2 = 8,
    Left1 = 9,
    COUNT = 10
};

//---------------------------------------------------------
//   AlterationChord
//   Native sheet-only alteration chord annotation with central
//   larger arbitrary text (default I or IV) and 10 smaller string slots:
//   1 above/below, 2 on each side, and each corner.
//---------------------------------------------------------

class AlterationChord final : public TextBase
{
    OBJECT_ALLOCATOR(engraving, AlterationChord)
    DECLARE_CLASSOF(ElementType::ALTERATION_CHORD)

public:
    static constexpr int FOCUS_CENTRAL = -1;
    static constexpr int SLOTS_COUNT = 10;
    // Keep native XML field IDs stable for scores saved with the twelve-field layout.
    static constexpr std::array<int, SLOTS_COUNT> XML_SLOT_IDS { 0, 1, 3, 4, 5, 6, 8, 9, 10, 11 };

    AlterationChord(DummyParentOr<Segment> parent, TextStyleType tid = TextStyleType::STAFF);
    AlterationChord(const AlterationChord& other);

    AlterationChord* clone() const override { return new AlterationChord(*this); }
    EngravingItem* linkedClone() override;

    bool isEditAllowed(EditData& ed) const override;
    bool edit(EditData& ed) override;
    void startEdit(EditData& ed) override;
    void endEdit(EditData& ed) override;
    bool mousePress(EditData& ed) override;

    bool positionRelativeToNoteheadRest() const override { return true; }

    Segment* segment() const;

    String centralText() const { return plainText(); }
    void setCentralText(const String& s);

    String slotText(int slot) const;
    void setSlotText(int slot, const String& text);
    void undoSetSlotText(int slot, const String& text);

    const std::array<String, SLOTS_COUNT>& allSlots() const { return m_slots; }
    void setSlots(const std::array<String, SLOTS_COUNT>& newSlots);

    bool isSlotSelected() const { return m_slotSelected; }
    void setSlotSelected(bool sel) { m_slotSelected = sel; }
    void selectAll() override;
    bool deleteSelectedText() override;
    void editCopy(EditData& ed) override;
    void paste(const String& txt) override;
    void insertText(const String& text) override;
    void insertSym(SymId id) override;
    EngravingItem* drop(Transaction& tx, EditData& ed) override;

    struct SlotTextRun {
        String text;
        muse::draw::Font font;
        PointF pos;
    };

    int activeSlot() const { return m_activeSlot; }
    void setActiveSlot(int slot) { m_activeSlot = slot; m_slotCursor = -1; }
    size_t slotCursorPosition() const;
    RectF slotCursorRect() const;

    void nextSlot();
    void prevSlot();

    void added() override;
    void removed() override;

    struct LayoutData : public TextBase::LayoutData {
        RectF centralRect;
        std::array<RectF, SLOTS_COUNT> slotRect;
        double slotSize = 0.0;
        std::array<std::vector<SlotTextRun>, SLOTS_COUNT> slotTextRuns;
    };
    DECLARE_LAYOUTDATA_METHODS(AlterationChord)

    static void calculateSlotsLayout(LayoutData* ldata, double sp, const AlterationChord* item = nullptr);

private:
    void focusSlotAt(int slot, double x);
    std::array<String, SLOTS_COUNT> m_slots;
    int m_activeSlot = FOCUS_CENTRAL;
    bool m_slotSelected = false;
    int m_slotCursor = -1;
};

inline AlterationChord* toAlterationChord(EngravingItem* item)
{
    return item && item->type() == ElementType::ALTERATION_CHORD ? static_cast<AlterationChord*>(item) : nullptr;
}

inline const AlterationChord* toAlterationChord(const EngravingItem* item)
{
    return item && item->type() == ElementType::ALTERATION_CHORD ? static_cast<const AlterationChord*>(item) : nullptr;
}
} // namespace mu::engraving

#endif // MU_ENGRAVING_ALTERATIONCHORD_H
