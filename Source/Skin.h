#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

struct SkinPalette
{
    const char* id;
    const char* name;
    const char* subtitle;
    juce::uint32 bg, surface, surface2, border, text, muted, accent, accent2, mute, solo, fader;
};

// Touhou / Vocaloid / Japanese palettes plus the hardware replica.
inline constexpr int kNumSkins = 9;

inline const SkinPalette kSkins[kNumSkins] = {
    { "studio",   "Studio Canvas",    "Original graphite",     0xff101218, 0xff1a1d27, 0xff232734, 0xff323646, 0xffece8df, 0xff8b8f9c, 0xffe8a317, 0xff3dbaa0, 0xffc4453c, 0xff3db86e, 0xffd8c9a0 },
    { "asgod",    "As God Intended",  "SD-80 hardware silver", 0xffc8c4bc, 0xffb5b1a8, 0xffddd8ce, 0xff8a8680, 0xff1c1c1c, 0xff5a5854, 0xff1e4a8c, 0xff1f7a6c, 0xffa33b2b, 0xff2e8b4a, 0xff3a3a3c },
    { "scarlett", "Scarlett",         "Flandre Scarlet",       0xff1a0a0c, 0xff2b1014, 0xff3a161c, 0xff5c222c, 0xffffe8c8, 0xffc9a090, 0xffe8c547, 0xffc41e3a, 0xff8b1538, 0xffe8c547, 0xffffd36a },
    { "baguette", "Baguette",         "Kasane Teto",           0xff1c1014, 0xff2a181e, 0xff3b2228, 0xff5a333c, 0xffffe4ea, 0xffc9a0a8, 0xffe23d28, 0xffff8fab, 0xffe23d28, 0xff7dcfb6, 0xffffc2d1 },
    { "leek",     "Leek",             "Hatsune Miku",          0xff0c1416, 0xff122022, 0xff1a2e32, 0xff2a4a50, 0xffe5f8f7, 0xff7aa8a6, 0xff39c5bb, 0xff137a7f, 0xffc4453c, 0xff39c5bb, 0xffb8fff8 },
    { "hakurei",  "Hakurei",          "Reimu Hakurei",         0xff140c0c, 0xff221010, 0xff321818, 0xff5a2a28, 0xfffff4e8, 0xffc4b0a0, 0xffc23a2b, 0xffe8d5a3, 0xffc23a2b, 0xff3db86e, 0xfff0e0b0 },
    { "lunatic",  "Lunatic",          "Cirno",                 0xff0a1220, 0xff122038, 0xff1a2e4c, 0xff2a4870, 0xffe8f4ff, 0xff8aa8c8, 0xff6ec6ff, 0xfffff4a3, 0xffff6b6b, 0xff6ec6ff, 0xfffff4a3 },
    { "sakura",   "Sakura",           "Night blossom",         0xff140c16, 0xff221428, 0xff321e3a, 0xff4a2e55, 0xffffe8f0, 0xffc4a0b4, 0xffff8fab, 0xffc084fc, 0xffc4453c, 0xfff9a8d4, 0xffffc2d4 },
    { "matcha",   "Matcha",           "Wabi-sabi green",       0xff12140c, 0xff1c2014, 0xff2a3220, 0xff3e4a30, 0xfff3ead3, 0xffa8b090, 0xff7d9b4e, 0xffc4b07a, 0xffa33b2b, 0xff7d9b4e, 0xffd4c8a0 },
};

inline int skinIndexForId(const juce::String& id)
{
    for (int i = 0; i < kNumSkins; ++i)
        if (id == kSkins[i].id)
            return i;
    return 0;
}
