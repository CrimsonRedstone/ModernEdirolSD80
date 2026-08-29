#pragma once

#include <cmath>
#include <juce_gui_basics/juce_gui_basics.h>
#include "Skin.h"

class CassetteDeck : public juce::Component, private juce::Timer
{
public:
    std::function<void()> onPlay, onPause, onStop, onLoad, onApplySetup;
    std::function<void(bool)> onLoop;

    CassetteDeck()
    {
        play.setButtonText("PLAY");
        pause.setButtonText("PAUSE");
        stop.setButtonText("STOP");
        loop.setButtonText("LOOP");
        loop.setClickingTogglesState(true);
        load.setButtonText("LOAD TAPE");
        apply.setButtonText("Send setup to SD-80");
        for (auto* b : { &play, &pause, &stop, &loop, &load, &apply })
        {
            b->addListener(&buttonRelay);
            addAndMakeVisible(*b);
        }
        buttonRelay.owner = this;
        startTimerHz(60);
    }

    void setLooping(bool v) { loop.setToggleState(v, juce::dontSendNotification); }
    bool isLooping() const { return loop.getToggleState(); }

    juce::TextButton play, pause, stop, loop, load, apply;

    void setPalette(const SkinPalette& p) { pal = &p; repaint(); }

    void setState(bool loadedIn, bool playingIn, const juce::String& titleIn,
                  double pos, double len)
    {
        loaded = loadedIn;
        playing = playingIn;
        title = titleIn;
        position = pos;
        length = len;
        reelSpeed = playing ? 0.055f : 0.0f;
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

        auto r = getLocalBounds().toFloat().reduced(8.0f);
        g.setColour(cSurf);
        g.fillRoundedRectangle(r, 10.0f);
        g.setColour(cBorder);
        g.drawRoundedRectangle(r, 10.0f, 1.0f);

        auto window = r.reduced(24.0f, 18.0f).withHeight(juce::jmin(200.0f, r.getHeight() - 80.0f));
        g.setColour(cBg.darker(0.2f));
        g.fillRoundedRectangle(window, 8.0f);
        g.setColour(cAcc.withAlpha(0.35f));
        g.drawRoundedRectangle(window, 8.0f, 1.5f);

        if (! loaded)
        {
            g.setColour(cMut);
            g.setFont(juce::FontOptions(18.0f));
            g.drawText("NO TAPE", window, juce::Justification::centred);
            g.setFont(juce::FontOptions(13.0f));
            g.drawText("Load a .mid file to drop a cassette in the bay",
                       window.translated(0, 26), juce::Justification::centred);
        }
        else
        {
            auto body = window.reduced(36.0f, 28.0f);
            g.setColour(cSurf2);
            g.fillRoundedRectangle(body, 6.0f);
            g.setColour(cAcc);
            g.drawRoundedRectangle(body, 6.0f, 1.2f);

            const float reelR = juce::jmin(28.0f, body.getHeight() * 0.28f);
            auto left = juce::Point<float>(body.getX() + body.getWidth() * 0.28f, body.getCentreY() + 8.0f);
            auto right = juce::Point<float>(body.getX() + body.getWidth() * 0.72f, body.getCentreY() + 8.0f);
            drawReel(g, left, reelR, reelAngle, cAcc, cBg);
            drawReel(g, right, reelR, reelAngle, cAcc2, cBg);

            g.setColour(cAcc.withAlpha(0.7f));
            g.drawLine(left.x + reelR, left.y, right.x - reelR, right.y, 2.0f);

            g.setColour(cText);
            g.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
            g.drawText(title, body.removeFromTop(28.0f), juce::Justification::centred);

            const auto label = formatTime(position) + "  /  " + formatTime(length);
            g.setColour(cMut);
            g.setFont(juce::FontOptions(11.0f));
            g.drawText(label,
                       (int) window.getX(),
                       (int) (window.getBottom() - 22.0f),
                       (int) window.getWidth(),
                       18,
                       juce::Justification::centred);
        }
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(16);
        auto row = r.removeFromBottom(40);
        const int w = row.getWidth() / 6;
        play.setBounds(row.removeFromLeft(w).reduced(4, 4));
        pause.setBounds(row.removeFromLeft(w).reduced(4, 4));
        stop.setBounds(row.removeFromLeft(w).reduced(4, 4));
        loop.setBounds(row.removeFromLeft(w).reduced(4, 4));
        load.setBounds(row.removeFromLeft(w).reduced(4, 4));
        apply.setBounds(row.reduced(4, 4));
    }

    void timerCallback() override
    {
        if (reelSpeed > 0.0f)
        {
            reelAngle += reelSpeed;
            if (reelAngle > juce::MathConstants<float>::twoPi)
                reelAngle -= juce::MathConstants<float>::twoPi;
            repaint();
        }
    }

private:
    struct Relay : public juce::Button::Listener
    {
        CassetteDeck* owner = nullptr;
        void buttonClicked(juce::Button* b) override
        {
            if (owner == nullptr) return;
            if (b == &owner->play && owner->onPlay) owner->onPlay();
            if (b == &owner->pause && owner->onPause) owner->onPause();
            if (b == &owner->stop && owner->onStop) owner->onStop();
            if (b == &owner->load && owner->onLoad) owner->onLoad();
            if (b == &owner->apply && owner->onApplySetup) owner->onApplySetup();
            if (b == &owner->loop && owner->onLoop) owner->onLoop(owner->loop.getToggleState());
        }
    } buttonRelay;

    static void drawReel(juce::Graphics& g, juce::Point<float> c, float r, float ang,
                         juce::Colour accent, juce::Colour bg)
    {
        g.setColour(bg);
        g.fillEllipse(c.x - r, c.y - r, r * 2, r * 2);
        g.setColour(accent);
        g.drawEllipse(c.x - r, c.y - r, r * 2, r * 2, 2.0f);
        for (int i = 0; i < 6; ++i)
        {
            const float a = ang + i * juce::MathConstants<float>::twoPi / 6.0f;
            g.drawLine(c.x, c.y, c.x + std::cos(a) * (r - 4.0f), c.y + std::sin(a) * (r - 4.0f), 1.4f);
        }
        g.fillEllipse(c.x - 4.0f, c.y - 4.0f, 8.0f, 8.0f);
    }

    static juce::String formatTime(double sec)
    {
        sec = juce::jmax(0.0, sec);
        const int s = (int) sec;
        return juce::String(s / 60).paddedLeft('0', 2) + ":" + juce::String(s % 60).paddedLeft('0', 2);
    }

    const SkinPalette* pal { &kSkins[0] };
    bool loaded { false };
    bool playing { false };
    juce::String title;
    double position { 0 }, length { 0 };
    float reelAngle { 0 }, reelSpeed { 0 };
};
