#pragma once

#include <cmath>
#include <cstdint>
#include <vector>
#include <juce_gui_basics/juce_gui_basics.h>
#include "MidiPlayer.h"
#include "Skin.h"

// Piano-roll under the cassette. Empty until a tape is loaded.
// Whole SMF is shown; the playhead travels with playback. One colour per channel.
class MidiRoll : public juce::Component, private juce::Timer
{
public:
    MidiRoll() { setOpaque(false); }

    ~MidiRoll() override { stopTimer(); }

    void attach(MidiPlayerEngine* e) { engine = e; }

    void setActive(bool v)
    {
        if (v == active)
            return;
        active = v;
        if (v)
            startTimerHz(60);
        else
            stopTimer();
        repaint();
    }

    void setPalette(const SkinPalette& p) { pal = &p; repaint(); }

    void setPartNames(const juce::StringArray& names16) { partNames = names16; }

    void setSilencedMask(std::uint32_t mask16) { silenced = mask16; }

    void paint(juce::Graphics& g) override
    {
        if (engine != nullptr)
            engine->copyScore(seenGen, score, scoreLength, loNote, hiNote, channelMask);

        auto cBg = juce::Colour(pal ? pal->bg : 0xff101218);
        auto cSurf = juce::Colour(pal ? pal->surface : 0xff1a1d27);
        auto cAcc = juce::Colour(pal ? pal->accent : 0xffe8a317);
        auto cAcc2 = juce::Colour(pal ? pal->accent2 : 0xff3dbaa0);
        auto cText = juce::Colour(pal ? pal->text : 0xffece8df);
        auto cMut = juce::Colour(pal ? pal->muted : 0xff8b8f9c);
        auto cBorder = juce::Colour(pal ? pal->border : 0xff323646);

        // Same 8 px inset as CassetteDeck so the two rounded frames line up.
        auto r = getLocalBounds().toFloat().reduced(8.0f);
        g.setColour(cSurf);
        g.fillRoundedRectangle(r, 10.0f);
        g.setColour(cBorder);
        g.drawRoundedRectangle(r, 10.0f, 1.0f);

        const bool haveTape = engine != nullptr && engine->isLoaded();
        const std::uint32_t mask = haveTape ? channelMask : 0;
        const int used = countBits(mask);
        const int legendH = (! haveTape) ? 22 : (used > 8 ? 38 : 22);

        auto inner = r.reduced(12.0f, 10.0f);
        auto caption = inner.removeFromTop(16.0f);
        auto legend = inner.removeFromBottom((float) legendH);
        inner.removeFromTop(4.0f);
        inner.removeFromBottom(4.0f);

        g.setColour(cMut);
        g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
        juce::String cap;
        if (! haveTape)
            cap = "Load a .mid - the arrangement appears here";
        else
            cap = juce::String(used) + " colours  |  "
                + juce::String((int) score.size()) + " notes  |  "
                + (lastPlaying ? "PLAYING" : "CUED");
        g.drawText(cap, caption.toNearestInt(), juce::Justification::centredLeft);
        g.setColour(cAcc.withAlpha(0.85f));
        g.setFont(juce::FontOptions(10.0f));
        g.drawText("PIANO ROLL", caption.toNearestInt(), juce::Justification::centredRight);

        auto well = inner;
        const auto wellFill = cBg.interpolatedWith(juce::Colours::black, 0.72f);
        g.setColour(wellFill);
        g.fillRoundedRectangle(well, 7.0f);
        g.setColour(cAcc.withAlpha(0.28f));
        g.drawRoundedRectangle(well, 7.0f, 1.0f);

        auto keys = well.removeFromLeft(36.0f);
        auto plot = well.reduced(2.0f, 3.0f);

        const int lo = haveTape ? loNote : 24;
        const int hi = haveTape ? hiNote : 96;
        const int spanLo = juce::jmax(0, lo - 2);
        const int spanHi = juce::jmin(127, hi + 2);
        const int span = juce::jmax(18, spanHi - spanLo);

        bool sounding[128];
        juce::uint32 soundCol[128];
        for (int i = 0; i < 128; ++i)
        {
            sounding[i] = false;
            soundCol[i] = 0;
        }

        const double length = juce::jmax(0.001, haveTape ? scoreLength : 1.0);
        const double pos = juce::jlimit(0.0, length, displayPos);
        const double t0 = 0.0;
        const double windowSec = length;

        if (haveTape)
        {
            for (const auto& hit : score)
            {
                if (pos < hit.startSec || pos >= hit.endSec)
                    continue;
                if (hit.note < 0 || hit.note > 127)
                    continue;
                if ((silenced & (1u << (hit.channel - 1))) != 0)
                    continue;
                sounding[hit.note] = true;
                soundCol[hit.note] = colourForChannel(hit.channel).getARGB();
            }
        }

        drawKeyboard(g, keys, spanLo, span, wellFill, cMut, sounding, soundCol);

        {
            juce::Graphics::ScopedSaveState clip(g);
            g.reduceClipRegion(plot.toNearestInt());

            g.setColour(cMut.withAlpha(0.12f));
            const double step = gridStep(length);
            for (double t = 0.0; t <= length + 0.001; t += step)
            {
                const float x = xAt(plot, t0, windowSec, t);
                g.drawVerticalLine((int) x, plot.getY(), plot.getBottom());
            }
            for (int n = spanLo; n <= spanLo + span; ++n)
            {
                if ((n % 12) != 0)
                    continue;
                g.setColour(cMut.withAlpha(0.10f));
                g.drawHorizontalLine((int) yAt(plot, spanLo, span, n),
                                     plot.getX(), plot.getRight());
            }

            if (haveTape)
            {
                const float noteH = juce::jmax(2.4f, plot.getHeight() / (float) span - 0.5f);
                int drawn = 0;
                for (const auto& hit : score)
                {
                    if (hit.endSec < t0 || hit.startSec > t0 + windowSec)
                        continue;
                    if (++drawn > 8000)
                        break;
                    const bool on = (pos >= hit.startSec && pos < hit.endSec)
                        && (silenced & (1u << (hit.channel - 1))) == 0;
                    if (on)
                        continue; // second pass
                    drawHit(g, plot, t0, windowSec, spanLo, span, noteH, hit, false, silenced);
                }
                drawn = 0;
                for (const auto& hit : score)
                {
                    const bool on = (pos >= hit.startSec && pos < hit.endSec)
                        && (silenced & (1u << (hit.channel - 1))) == 0;
                    if (! on)
                        continue;
                    if (++drawn > 256)
                        break;
                    drawHit(g, plot, t0, windowSec, spanLo, span, noteH, hit, true, silenced);
                }

                const float px = xAt(plot, t0, windowSec, pos);
                g.setColour(cAcc.withAlpha(0.16f));
                g.fillRect(px - 7.0f, plot.getY(), 14.0f, plot.getHeight());
                g.setColour(cAcc);
                g.fillRect(px - 1.5f, plot.getY(), 3.0f, plot.getHeight());
                g.setColour(cAcc2);
                g.fillEllipse(px - 5.0f, plot.getY() - 1.0f, 10.0f, 10.0f);
            }
            else
            {
                g.setColour(cMut.withAlpha(0.7f));
                g.setFont(juce::FontOptions(14.0f));
                g.drawText("NO TAPE", plot.toNearestInt(), juce::Justification::centred);
            }
        }

        drawLegend(g, legend, mask, cMut, cText, haveTape);
    }

    void timerCallback() override
    {
        if (engine != nullptr)
            engine->copyScore(seenGen, score, scoreLength, loNote, hiNote, channelMask);

        const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
        if (engine != nullptr && engine->isLoaded())
        {
            const double eng = engine->getPosition();
            const bool playing = engine->isPlaying();
            const double len = juce::jmax(0.001, scoreLength);
            if (playing)
            {
                if (! lastPlaying)
                    displayPos = eng;
                else if (eng + 0.4 < displayPos)
                    displayPos = eng; // looped
                else
                {
                    displayPos += now - lastWallSec;
                    if (std::abs(displayPos - eng) > 0.18)
                        displayPos = eng;
                }
                displayPos = juce::jlimit(0.0, len, displayPos);
            }
            else
            {
                displayPos = eng;
            }
            lastPlaying = playing;
        }
        else
        {
            lastPlaying = false;
            displayPos = 0.0;
        }
        lastWallSec = now;
        repaint();
    }

    static juce::Colour colourForChannel(int ch1to16)
    {
        // 16 discrete hues/lightnesses - no golden-angle collisions (A3/A8 used to match).
        static const juce::uint32 kCol[16] = {
            0xffff3b30, // 1  red
            0xff0a84ff, // 2  blue
            0xff30d158, // 3  green
            0xffbf5af2, // 4  purple
            0xffff9f0a, // 5  orange
            0xff64d2ff, // 6  cyan
            0xffffd60a, // 7  yellow
            0xffff375f, // 8  rose
            0xffac8e68, // 9  tan
            0xff7dffb3, // 10 mint
            0xff5e5ce6, // 11 indigo
            0xffff6482, // 12 coral
            0xff00c7be, // 13 teal
            0xffd0ff00, // 14 lime
            0xffda8fff, // 15 lavender
            0xff8e8e93  // 16 grey
        };
        const int i = juce::jlimit(0, 15, ch1to16 - 1);
        return juce::Colour(kCol[i]);
    }

private:
    static float xAt(juce::Rectangle<float> plot, double t0, double window, double t)
    {
        if (window <= 0.0)
            return plot.getX();
        return plot.getX() + (float) ((t - t0) / window) * plot.getWidth();
    }

    static float yAt(juce::Rectangle<float> plot, int spanLo, int span, int note)
    {
        const float n = (float) (note - spanLo);
        return plot.getBottom() - (n / (float) span) * plot.getHeight();
    }

    static int countBits(std::uint32_t m)
    {
        int n = 0;
        while (m) { n += (int) (m & 1u); m >>= 1; }
        return n;
    }

    static double gridStep(double len)
    {
        if (len <= 8.0)  return 1.0;
        if (len <= 20.0) return 2.0;
        if (len <= 45.0) return 5.0;
        if (len <= 90.0) return 10.0;
        if (len <= 180.0) return 15.0;
        return 30.0;
    }

    static bool isBlackKey(int note)
    {
        const int pc = ((note % 12) + 12) % 12;
        return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
    }

    static void drawHit(juce::Graphics& g, juce::Rectangle<float> plot,
                        double t0, double window, int spanLo, int span, float noteH,
                        const PlayerNote& hit, bool on, std::uint32_t silenced)
    {
        const float x0 = xAt(plot, t0, window, hit.startSec);
        const float x1 = juce::jmax(x0 + 2.0f, xAt(plot, t0, window, hit.endSec));
        const float h = on ? noteH + 2.0f : noteH;
        const float y = yAt(plot, spanLo, span, hit.note) - h * 0.5f;
        const bool dim = (silenced & (1u << (hit.channel - 1))) != 0;
        auto col = colourForChannel(hit.channel);
        if (dim)
            col = col.withMultipliedAlpha(0.22f);
        if (on)
        {
            g.setColour(col.withAlpha(0.28f));
            g.fillRoundedRectangle(x0 - 1.5f, y - 1.5f, (x1 - x0) + 3.0f, h + 3.0f, 3.0f);
            g.setColour(col.brighter(0.18f));
            g.fillRoundedRectangle(x0, y, x1 - x0, h, 2.0f);
            g.setColour(juce::Colours::white);
            g.fillRect(x0, y, juce::jmin(3.0f, x1 - x0), h);
        }
        else
        {
            g.setColour(col.withAlpha(dim ? 0.20f : 0.72f));
            g.fillRoundedRectangle(x0, y, x1 - x0, h, 2.0f);
        }
    }

    static void drawKeyboard(juce::Graphics& g, juce::Rectangle<float> keys,
                             int spanLo, int span, juce::Colour well, juce::Colour mut,
                             const bool sounding[128], const juce::uint32 soundCol[128])
    {
        const float h = keys.getHeight() / (float) span;
        for (int i = 0; i < span; ++i)
        {
            const int note = spanLo + i;
            const float y = keys.getBottom() - (float) (i + 1) * h;
            const float kh = juce::jmax(1.0f, h - 0.5f);
            const bool black = isBlackKey(note);
            const bool on = (note >= 0 && note < 128) ? sounding[note] : false;
            if (on)
            {
                auto col = juce::Colour(soundCol[note]);
                g.setColour(black ? col.darker(0.15f) : col);
                g.fillRect(keys.getX(), y, keys.getWidth(), kh);
                g.setColour(juce::Colours::white.withAlpha(0.55f));
                g.fillRect(keys.getX(), y, 3.0f, kh);
            }
            else
            {
                g.setColour(black ? well.darker(0.35f) : well.brighter(0.18f));
                g.fillRect(keys.getX(), y, keys.getWidth(), kh);
            }
            if ((note % 12) == 0)
            {
                g.setColour(on ? juce::Colours::white : mut.withAlpha(0.9f));
                g.setFont(juce::FontOptions(9.0f));
                g.drawText("C" + juce::String(note / 12 - 1),
                           (int) keys.getX() + 4, (int) y, (int) keys.getWidth() - 6,
                           (int) juce::jmax(h, 10.0f),
                           juce::Justification::centredLeft);
            }
        }
        g.setColour(mut.withAlpha(0.35f));
        g.drawVerticalLine((int) keys.getRight(), keys.getY(), keys.getBottom());
    }

    void drawLegend(juce::Graphics& g, juce::Rectangle<float> legend,
                    std::uint32_t mask, juce::Colour mut, juce::Colour text, bool haveTape)
    {
        int used = 0;
        int channels[16];
        for (int ch = 1; ch <= 16; ++ch)
            if ((mask & (1u << (ch - 1))) != 0)
                channels[used++] = ch;
        if (used == 0)
        {
            g.setColour(mut);
            g.setFont(juce::FontOptions(11.0f));
            g.drawText("Nothing on the tape yet", legend.toNearestInt(), juce::Justification::centred);
            return;
        }
        const int cols = juce::jmin(8, used);
        const int rows = (used + cols - 1) / cols;
        const float rowH = legend.getHeight() / (float) rows;
        const float slotW = legend.getWidth() / (float) cols;
        for (int i = 0; i < used; ++i)
        {
            const int ch = channels[i];
            const int row = i / cols;
            const int col = i % cols;
            auto slotR = juce::Rectangle<float>(legend.getX() + slotW * (float) col,
                                                legend.getY() + rowH * (float) row,
                                                slotW, rowH);
            auto chip = slotR.removeFromLeft(10.0f).withSizeKeepingCentre(8.0f, 8.0f);
            auto colr = colourForChannel(ch);
            if (haveTape && (silenced & (1u << (ch - 1))) != 0)
                colr = colr.withMultipliedAlpha(0.3f);
            g.setColour(colr);
            g.fillEllipse(chip);
            g.setColour(text.withAlpha(0.9f));
            g.setFont(juce::FontOptions(10.0f));
            juce::String label = "A" + juce::String(ch);
            if (haveTape && ch - 1 < partNames.size())
            {
                const auto n = partNames[ch - 1];
                if (n.isNotEmpty())
                    label += " " + n;
            }
            g.drawText(label, slotR.reduced(4.0f, 0).toNearestInt(), juce::Justification::centredLeft);
        }
    }

    MidiPlayerEngine* engine { nullptr };
    const SkinPalette* pal { &kSkins[0] };
    juce::StringArray partNames;
    std::vector<PlayerNote> score;
    double scoreLength { 0.0 };
    double displayPos { 0.0 };
    double lastWallSec { 0.0 };
    int loNote { 48 }, hiNote { 72 };
    int seenGen { -1 };
    std::uint32_t channelMask { 0 };
    std::uint32_t silenced { 0 };
    bool active { false };
    bool lastPlaying { false };
};
