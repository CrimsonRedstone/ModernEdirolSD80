#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Right-click lock overlay for sliders, buttons and combo boxes.
// Locked controls keep their value through patch changes, MIDI import and preset load.
namespace sd80lock
{
inline void drawIcon(juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour colour)
{
    auto r = bounds.removeFromRight(14).removeFromTop(14).reduced(1).toFloat();
    if (r.getWidth() < 6.0f)
        return;
    g.setColour(colour);
    const float cx = r.getCentreX();
    const float bodyY = r.getY() + r.getHeight() * 0.42f;
    juce::Path shackle;
    shackle.addCentredArc(cx, bodyY, 3.2f, 3.2f, 0.0f,
                          juce::MathConstants<float>::pi,
                          juce::MathConstants<float>::twoPi, true);
    g.strokePath(shackle, juce::PathStrokeType(1.4f));
    g.fillRoundedRectangle(cx - 4.2f, bodyY, 8.4f, 6.4f, 1.2f);
}

template <typename Base>
class Ctrl : public Base
{
public:
    using Base::Base;
    Ctrl() = default;
    explicit Ctrl(const juce::String& name) : Base(name) {}
    Ctrl(const char* name) : Base(juce::String(name)) {}

    juce::String paramId;
    std::function<bool(const juce::String&)> lockedFn;
    std::function<void(const juce::String&)> toggleFn;
    juce::Colour lockColour { juce::Colour(0xffe8a317) };

    bool isCurrentlyLocked() const
    {
        return paramId.isNotEmpty() && lockedFn && lockedFn(paramId);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            if (toggleFn && paramId.isNotEmpty())
                toggleFn(paramId);
            return;
        }
        if (isCurrentlyLocked())
            return;
        Base::mouseDown(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (isCurrentlyLocked())
            return;
        Base::mouseDrag(e);
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (isCurrentlyLocked())
            return;
        Base::mouseUp(e);
    }

    void mouseDoubleClick(const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() || isCurrentlyLocked())
            return;
        Base::mouseDoubleClick(e);
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        if (isCurrentlyLocked())
            return;
        Base::mouseWheelMove(e, w);
    }

    void paint(juce::Graphics& g) override
    {
        Base::paint(g);
        if (isCurrentlyLocked())
            drawIcon(g, this->getLocalBounds(), lockColour);
    }
};

using Slider = Ctrl<juce::Slider>;
using TextButton = Ctrl<juce::TextButton>;
using ComboBox = Ctrl<juce::ComboBox>;
using ToggleButton = Ctrl<juce::ToggleButton>;
using Label = Ctrl<juce::Label>;
} // namespace sd80lock
