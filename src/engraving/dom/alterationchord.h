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
#include "textbase.h"

namespace mu::engraving {
class Segment;

enum class AlterationSlot : int {
    TopLeft = 0,
    Top1 = 1,
    Top2 = 2,
    TopRight = 3,
    Right1 = 4,
    Right2 = 5,
    BottomRight = 6,
    Bottom2 = 7,
    Bottom1 = 8,
    BottomLeft = 9,
    Left2 = 10,
    Left1 = 11,
    COUNT = 12
};

//---------------------------------------------------------
//   AlterationChord
//   Native sheet-only alteration chord annotation with central
//   larger arbitrary text (default I or IV) and 12 smaller string slots:
//   2 centered on each edge + each corner.
//---------------------------------------------------------

class AlterationChord final : public TextBase
{
    OBJECT_ALLOCATOR(engraving, AlterationChord)
    DECLARE_CLASSOF(ElementType::ALTERATION_CHORD)

public:
    static constexpr int FOCUS_CENTRAL = -1;
    static constexpr int SLOTS_COUNT = 12;

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

    int activeSlot() const { return m_activeSlot; }
    void setActiveSlot(int slot) { m_activeSlot = slot; }

    void nextSlot();
    void prevSlot();

    void added() override;
    void removed() override;

    struct LayoutData : public TextBase::LayoutData {
        RectF centralRect;
        std::array<RectF, SLOTS_COUNT> slotRect;
        double slotSize = 0.0;
    };
    DECLARE_LAYOUTDATA_METHODS(AlterationChord)

    static void calculateSlotsLayout(LayoutData* ldata, double sp, const AlterationChord* item = nullptr);

private:
    std::array<String, SLOTS_COUNT> m_slots;
    int m_activeSlot = FOCUS_CENTRAL;
    bool m_slotSelected = false;
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
