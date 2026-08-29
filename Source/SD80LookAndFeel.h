#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Skin.h"

class SD80LookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Colour bg, surface, surface2, border, text, muted, amber, teal, muteRed, soloGreen, fader;
    int skinIndex { 0 };

    SD80LookAndFeel() { applySkin(0); }

    void applySkin(int index)
    {
        skinIndex = juce::jlimit(0, kNumSkins - 1, index);
        const auto& s = kSkins[skinIndex];
        bg        = juce::Colour(s.bg);
        surface   = juce::Colour(s.surface);
        surface2  = juce::Colour(s.surface2);
        border    = juce::Colour(s.border);
        text      = juce::Colour(s.text);
        muted     = juce::Colour(s.muted);
        amber     = juce::Colour(s.accent);
        teal      = juce::Colour(s.accent2);
        muteRed   = juce::Colour(s.mute);
        soloGreen = juce::Colour(s.solo);
        fader     = juce::Colour(s.fader);
        applyColours();
    }

    juce::Font getTextButtonFont(juce::TextButton&, int) override
    {
        return juce::FontOptions(12.0f).withStyle("Bold");
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override
    {
        return juce::FontOptions(13.0f);
    }

    juce::Font getPopupMenuFont() override
    {
        return juce::FontOptions(13.0f);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                          float pos, float startAng, float endAng, juce::Slider& slider) override
    {
        auto r = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(3.0f);
        const auto radius = juce::jmin(r.getWidth(), r.getHeight()) * 0.5f;
        const auto cx = r.getCentreX();
        const auto cy = r.getCentreY();
        const auto rw = radius * 0.78f;

        g.setColour(surface2);
        g.fillEllipse(cx - rw, cy - rw, rw * 2, rw * 2);

        juce::Path arc;
        arc.addCentredArc(cx, cy, rw, rw, 0.0f, startAng, endAng, true);
        g.setColour(border);
        g.strokePath(arc, juce::PathStrokeType(3.0f));

        juce::Path value;
        value.addCentredArc(cx, cy, rw, rw, 0.0f, startAng, startAng + pos * (endAng - startAng), true);
        g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
        g.strokePath(value, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const auto ang = startAng + pos * (endAng - startAng);
        const auto pointerLen = rw * 0.62f;
        juce::Point<float> tip(cx + std::cos(ang - juce::MathConstants<float>::halfPi) * pointerLen,
                               cy + std::sin(ang - juce::MathConstants<float>::halfPi) * pointerLen);
        g.setColour(text);
        g.drawLine(cx, cy, tip.x, tip.y, 1.6f);
        g.fillEllipse(cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
                          float pos, float minPos, float maxPos, juce::Slider::SliderStyle style,
                          juce::Slider& slider) override
    {
        if (style != juce::Slider::LinearVertical)
        {
            LookAndFeel_V4::drawLinearSlider(g, x, y, w, h, pos, minPos, maxPos, style, slider);
            return;
        }
        auto track = juce::Rectangle<float>((float) x + w * 0.5f - 3.0f, (float) y, 6.0f, (float) h);
        g.setColour(surface2);
        g.fillRoundedRectangle(track, 3.0f);
        auto filled = track.withTop(pos);
        g.setColour(teal);
        g.fillRoundedRectangle(filled, 3.0f);
        g.setColour(fader);
        g.fillRoundedRectangle(juce::Rectangle<float>((float) x + 2, pos - 7.0f, (float) w - 4, 14.0f), 3.0f);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour& bgc,
                              bool highlighted, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced(0.5f);
        auto c = bgc;
        if (b.getToggleState())
            c = b.findColour(juce::TextButton::buttonOnColourId);
        if (highlighted) c = c.brighter(0.12f);
        if (down) c = c.darker(0.12f);
        g.setColour(c);
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(b.getToggleState() ? c.brighter(0.15f) : border);
        g.drawRoundedRectangle(r, 4.0f, 1.0f);
    }

    void drawTickBox(juce::Graphics& g, juce::Component& c, float x, float y, float w, float h,
                     bool ticked, bool, bool, bool) override
    {
        juce::ignoreUnused(c);
        juce::Rectangle<float> r(x, y, w, h);
        g.setColour(surface2);
        g.fillRoundedRectangle(r, 3.0f);
        g.setColour(border);
        g.drawRoundedRectangle(r, 3.0f, 1.0f);
        if (ticked)
        {
            g.setColour(teal);
            g.fillRoundedRectangle(r.reduced(3.0f), 2.0f);
        }
    }

private:
    void applyColours()
    {
        setColour(juce::ResizableWindow::backgroundColourId, bg);
        setColour(juce::Label::textColourId, text);
        setColour(juce::ComboBox::backgroundColourId, surface2);
        setColour(juce::ComboBox::textColourId, text);
        setColour(juce::ComboBox::outlineColourId, border);
        setColour(juce::ComboBox::arrowColourId, amber);
        setColour(juce::PopupMenu::backgroundColourId, surface);
        setColour(juce::PopupMenu::textColourId, text);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, teal.darker(0.35f));
        setColour(juce::PopupMenu::highlightedTextColourId, text);
        setColour(juce::TextButton::buttonColourId, surface2);
        setColour(juce::TextButton::buttonOnColourId, teal);
        setColour(juce::TextButton::textColourOffId, text);
        setColour(juce::TextButton::textColourOnId, bg);
        setColour(juce::Slider::rotarySliderFillColourId, amber);
        setColour(juce::Slider::rotarySliderOutlineColourId, border);
        setColour(juce::Slider::thumbColourId, fader);
        setColour(juce::Slider::trackColourId, teal);
        setColour(juce::Slider::backgroundColourId, surface2);
        setColour(juce::TextEditor::backgroundColourId, surface2);
        setColour(juce::TextEditor::textColourId, text);
        setColour(juce::TextEditor::outlineColourId, border);
        setColour(juce::TextEditor::highlightedTextColourId, bg);
        setColour(juce::TextEditor::highlightColourId, amber);
        setColour(juce::ListBox::backgroundColourId, bg);
        setColour(juce::ListBox::outlineColourId, border);
        setColour(juce::ListBox::textColourId, text);
        setColour(juce::ScrollBar::thumbColourId, border);
        setColour(juce::ToggleButton::textColourId, text);
        setColour(juce::ToggleButton::tickColourId, teal);
    }
};
