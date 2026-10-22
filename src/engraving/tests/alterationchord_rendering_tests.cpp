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
#include <vector>

#include "draw/bufferedpaintprovider.h"
#include "draw/painter.h"
#include "engraving/dom/alterationchord.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/segment.h"
#include "engraving/iengravingfont.h"
#include "engraving/editing/editdata.h"
#include "engraving/rendering/editmode/editmoderenderer.h"
#include "engraving/rendering/iscorerenderer.h"
#include "utils/scorerw.h"

using namespace mu::engraving;
using namespace muse::draw;

namespace {
struct PaintedText {
    String text;
    double fontSize;
    Font::Type fontType;
    Transform transform;
};

struct PaintSummary {
    std::vector<PaintedText> texts;
    size_t paths = 0;
    std::vector<Transform> pathTransforms;
    Transform finalTransform;
};

void collectPaint(const DrawData& data, const DrawData::Item& item, PaintSummary& result)
{
    for (const DrawData::Data& group : item.datas) {
        result.paths += group.paths.size();
        for (size_t i = 0; i < group.paths.size(); ++i) {
            result.pathTransforms.push_back(data.states.at(group.state).transform);
        }
        for (const DrawText& text : group.texts) {
            const auto& state = data.states.at(group.state);
            result.texts.push_back({ text.text, state.font.pointSizeF(), state.font.type(), state.transform });
        }
    }
    for (const DrawData::Item& child : item.chilren) {
        collectPaint(data, child, result);
    }
}

PaintSummary paintAnnotation(AlterationChord* chord, bool editingOverlay)
{
    auto provider = std::make_shared<BufferedPaintProvider>();
    Painter painter(provider, "alteration chord regression");
    rendering::PaintOptions options;
    options.isPrinting = !editingOverlay;
    const PointF pageOffset = chord->canvasPos() - chord->pagePos();
    painter.translate(pageOffset);
    chord->score()->renderer()->paintItem(painter, chord, options);
    painter.translate(-pageOffset);
    if (editingOverlay) {
        EditData editData;
        rendering::editmode::EditModeRenderer overlay;
        overlay.drawItem(chord, &painter, editData, 1.0, options);
    }
    const Transform finalTransform = painter.worldTransform();
    painter.endDraw();
    PaintSummary result;
    result.finalTransform = finalTransform;
    const DrawDataPtr data = provider->drawData();
    collectPaint(*data, data->item, result);
    return result;
}
}

class Engraving_AlterationChordRenderingTests : public ::testing::Test
{
protected:
    std::unique_ptr<MasterScore> score;
    std::unique_ptr<AlterationChord> chord;

    void SetUp() override
    {
        score.reset(ScoreRW::readScore(u"test.mscx"));
        ASSERT_NE(score, nullptr);
        Segment* segment = score->firstSegment(SegmentType::ChordRest);
        ASSERT_NE(segment, nullptr);
        chord.reset(Factory::createAlterationChord(segment));
        chord->setTrack(0);
        chord->setAutoplace(false);
        chord->setCentralText(u"IV");
    }

    void layout()
    {
        score->renderer()->layoutItem(static_cast<TextBase*>(chord.get()));
    }
};

TEST_F(Engraving_AlterationChordRenderingTests, PrintOmitsEmptySlotsAndPlaceholderOutlines)
{
    layout();
    const PaintSummary result = paintAnnotation(chord.get(), false);
    ASSERT_EQ(result.texts.size(), 1u);
    EXPECT_EQ(result.texts.front().text, u"IV");
    EXPECT_EQ(result.paths, 0u);
}

TEST_F(Engraving_AlterationChordRenderingTests, PrintDrawsEachLabelOnceInSmallerFont)
{
    chord->setSlotText(2, u"6♯");
    chord->setSlotText(8, u"♭");
    layout();
    const PaintSummary result = paintAnnotation(chord.get(), false);
    ASSERT_EQ(result.texts.size(), 3u);
    EXPECT_EQ(result.texts[0].text, u"IV");
    EXPECT_EQ(result.texts[1].text, u"6♯");
    EXPECT_EQ(result.texts[2].text, u"♭");
    EXPECT_LT(result.texts[1].fontSize, result.texts[0].fontSize);
    EXPECT_DOUBLE_EQ(result.texts[1].fontSize, result.texts[2].fontSize);
    EXPECT_EQ(result.paths, 0u);
}

TEST_F(Engraving_AlterationChordRenderingTests, EditOverlayAddsBoxesWithoutDuplicatingLabels)
{
    chord->setSlotText(2, u"6♯");
    layout();
    chord->setActiveSlot(2);
    const PaintSummary result = paintAnnotation(chord.get(), true);
    ASSERT_EQ(result.texts.size(), 2u);
    EXPECT_EQ(result.texts[0].text, u"IV");
    EXPECT_EQ(result.texts[1].text, u"6♯");
    EXPECT_GE(result.paths, 12u);
}

TEST_F(Engraving_AlterationChordRenderingTests, RotatedOverlayTracksLabelsAndRestoresPainter)
{
    chord->setSlotText(2, u"6♯");
    chord->setTextAngle(27.0);
    layout();
    chord->setPos(75.0, 45.0);
    chord->setActiveSlot(2);
    const PaintSummary result = paintAnnotation(chord.get(), true);
    ASSERT_EQ(result.texts.size(), 2u);
    ASSERT_GE(result.pathTransforms.size(), 12u);
    for (const Transform& transform : result.pathTransforms) {
        EXPECT_EQ(transform, result.texts[1].transform);
    }
    EXPECT_EQ(result.finalTransform, Transform());
}

TEST_F(Engraving_AlterationChordRenderingTests, MusicalSymbolAndDigitUseTheirRespectiveFonts)
{
    const String sharp = score->engravingFont()->toString(SymId::accidentalSharp);
    chord->setSlotText(4, sharp + u"6");
    layout();
    const auto& runs = chord->ldata()->slotTextRuns[4];
    ASSERT_EQ(runs.size(), 2u);
    EXPECT_EQ(runs[0].text, sharp);
    EXPECT_EQ(runs[1].text, u"6");
    const PaintSummary result = paintAnnotation(chord.get(), false);
    ASSERT_EQ(result.texts.size(), 3u);
    EXPECT_EQ(result.texts[1].text, sharp);
    EXPECT_EQ(result.texts[1].fontType, Font::Type::MusicSymbolText);
    EXPECT_EQ(result.texts[2].text, u"6");
    EXPECT_NE(result.texts[2].fontType, Font::Type::MusicSymbolText);
    EXPECT_DOUBLE_EQ(result.texts[1].fontSize, result.texts[2].fontSize);
}
