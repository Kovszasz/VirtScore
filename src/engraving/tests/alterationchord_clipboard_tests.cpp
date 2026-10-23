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

#include <memory>

#include "engraving/dom/alterationchord.h"
#include "engraving/dom/chordrest.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/segment.h"
#include "engraving/editing/paste.h"
#include "engraving/editing/transaction/transaction.h"
#include "engraving/editing/transaction/undostack.h"
#include "utils/scorerw.h"

using namespace mu::engraving;

TEST(Engraving_AlterationChordClipboardTests, SymbolListPastePreservesFieldsAndUndo)
{
    std::unique_ptr<MasterScore> score(ScoreRW::readScore(u"test.mscx"));
    ASSERT_NE(score, nullptr);
    Segment* source = score->firstSegment(SegmentType::ChordRest);
    ASSERT_NE(source, nullptr);
    Segment* destination = source->next1(SegmentType::ChordRest);
    ASSERT_NE(destination, nullptr);
    ChordRest* destinationCR = destination->cr(0);
    ASSERT_NE(destinationCR, nullptr);

    score->startCmd(TranslatableString::untranslatable("Add source annotations"));
    AlterationChord* first = score->addAlterationChord(source);
    AlterationChord* second = score->addAlterationChord(source);
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    first->setCentralText(u"IV");
    first->setSlotText(1, u"6♯");
    first->setSlotText(6, u"♭");
    second->setCentralText(u"V");
    second->setSlotText(2, u"<7&>");
    score->endCmd();

    score->select(first, SelectType::SINGLE);
    score->select(second, SelectType::ADD);
    ASSERT_TRUE(score->selection().canCopy());
    ASSERT_FALSE(score->selection().isSingle());
    muse::ByteArray data = score->selection().mimeData();
    ASSERT_FALSE(data.empty());

    score->select(destinationCR, SelectType::SINGLE);
    score->startCmd(TranslatableString::untranslatable("Paste annotations"));
    const bool pasteSucceeded = Paste::pasteSymbolList(score->transactionManager()->currentOrDummyTransaction(), score.get(), data);
    score->endCmd();
    ASSERT_TRUE(pasteSucceeded);
    score->doLayout();

    auto pasted = [&]() {
        std::vector<AlterationChord*> result;
        for (EngravingItem* item : destination->annotations()) {
            if (item->isAlterationChord()) {
                result.push_back(toAlterationChord(item));
            }
        }
        return result;
    };

    const auto annotations = pasted();
    ASSERT_EQ(annotations.size(), 2u);
    bool foundIV = false;
    bool foundV = false;
    for (const AlterationChord* chord : annotations) {
        EXPECT_NE(chord, first);
        EXPECT_NE(chord, second);
        if (chord->centralText() == u"IV") {
            foundIV = true;
            EXPECT_EQ(chord->slotText(1), u"6♯");
            EXPECT_EQ(chord->slotText(6), u"♭");
        } else if (chord->centralText() == u"V") {
            foundV = true;
            EXPECT_EQ(chord->slotText(2), u"<7&>");
        }
    }
    EXPECT_TRUE(foundIV);
    EXPECT_TRUE(foundV);

    score->undoStack()->undo(nullptr);
    EXPECT_TRUE(pasted().empty());
    EXPECT_EQ(first->slotText(1), u"6♯");
    EXPECT_EQ(second->slotText(2), u"<7&>");
    score->undoStack()->redo();
    EXPECT_EQ(pasted().size(), 2u);
}
