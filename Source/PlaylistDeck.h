#pragma once

#include <cmath>
#include <juce_gui_basics/juce_gui_basics.h>
#include "MidiPlayer.h"
#include "Skin.h"

// One Part A / Part B deck card on the PLAYLIST tab.
class PlaylistSlot : public juce::Component, private juce::Timer
{
public:
    PlaylistSlot() { setOpaque(false); }

    ~PlaylistSlot() override { stopTimer(); }

    void attach(MidiPlayerEngine* e) { engine = e; }
    void setPalette(const SkinPalette& p) { pal = &p; repaint(); }
    void setPortLabel(const juce::String& s) { port = s; repaint(); }
    void setActive(bool v)
    {
        if (v)
            startTimerHz(30);
        else
            stopTimer();
    }
    void setSpent(bool v)
    {
        if (spent == v)
            return;
        spent = v;
        repaint();
    }

    void timerCallback() override
    {
        if (engine != nullptr && engine->isPlaying())
            repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto cBg = juce::Colour(pal ? pal->bg : 0xff101218);
        auto cSurf = juce::Colour(pal ? pal->surface : 0xff1a1d27);
        auto cSurf2 = juce::Colour(pal ? pal->surface2 : 0xff232734);
        auto cAcc = juce::Colour(pal ? pal->accent : 0xffe8a317);
        auto cAcc2 = juce::Colour(pal ? pal->accent2 : 0xff3dbaa0);
        auto cText = juce::Colour(pal ? pal->text : 0xffece8df);
        auto cMut = juce::Colour(pal ? pal->muted : 0xff8b8f9c);
        auto cBorder = juce::Colour(pal ? pal->border : 0xff323646);

        const bool loaded = engine != nullptr && engine->isLoaded();
        const bool playing = loaded && engine->isPlaying();
        const juce::String name = loaded ? engine->getName() : juce::String();
        const double pos = loaded ? engine->getPosition() : 0.0;
        const double len = loaded ? engine->getLength() : 0.0;
        const float frac = (len > 0.05) ? (float) juce::jlimit(0.0, 1.0, pos / len) : 0.0f;

        juce::String badge = "EMPTY";
        juce::Colour badgeCol = cMut;
        if (playing)
        {
            badge = "PLAYING";
            badgeCol = cAcc2;
        }
        else if (loaded && spent)
        {
            badge = "DONE";
            badgeCol = cMut;
        }
        else if (loaded)
        {
            badge = "ARMED";
            badgeCol = cAcc;
        }

        auto r = getLocalBounds().toFloat().reduced(2.0f);
        g.setColour(playing ? cSurf2 : cSurf);
        g.fillRoundedRectangle(r, 10.0f);
        g.setColour(playing ? cAcc2.withAlpha(0.85f) : cBorder);
        g.drawRoundedRectangle(r, 10.0f, playing ? 1.8f : 1.0f);

        auto inner = r.reduced(16.0f, 14.0f);
        auto top = inner.removeFromTop(18.0f);
        g.setColour(cMut);
        g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
        g.drawText(port, top.removeFromLeft(top.getWidth() * 0.62f),
                   juce::Justification::centredLeft, false);

        auto pill = top.withSizeKeepingCentre(juce::jmin(88.0f, top.getWidth()), 16.0f);
        g.setColour(badgeCol.withAlpha(0.18f));
        g.fillRoundedRectangle(pill, 8.0f);
        g.setColour(badgeCol);
        g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
        g.drawText(badge, pill, juce::Justification::centred, false);

        inner.removeFromTop(10.0f);
        g.setColour(loaded ? cText : cMut);
        g.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
        const juce::String title = loaded ? name : juce::String("waiting for a file");
        g.drawText(title, inner.removeFromTop(24.0f), juce::Justification::centredLeft, true);

        inner.removeFromTop(6.0f);
        g.setColour(cMut);
        g.setFont(juce::FontOptions(12.0f));
        const juce::String times = loaded
            ? (fmtTime(pos) + "  /  " + fmtTime(len))
            : juce::String("--:--  /  --:--");
        g.drawText(times, inner.removeFromTop(16.0f), juce::Justification::centredLeft, false);

        inner.removeFromTop(10.0f);
        auto bar = inner.removeFromTop(6.0f);
        g.setColour(cBg);
        g.fillRoundedRectangle(bar, 3.0f);
        if (frac > 0.0f)
        {
            auto fill = bar.withWidth(juce::jmax(6.0f, bar.getWidth() * frac));
            g.setColour(playing ? cAcc2 : cAcc);
            g.fillRoundedRectangle(fill, 3.0f);
        }
    }

private:
    static juce::String fmtTime(double sec)
    {
        if (sec < 0.0)
            sec = 0.0;
        const int t = (int) std::llround(sec);
        return juce::String(t / 60) + ":" + juce::String(t % 60).paddedLeft('0', 2);
    }

    MidiPlayerEngine* engine { nullptr };
    const SkinPalette* pal { nullptr };
    juce::String port;
    bool spent { false };
};
