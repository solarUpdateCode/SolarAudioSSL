#include <juce_audio_utils/juce_audio_utils.h>
#include "SSLDSP.h"
#include <array>
#include <atomic>
#include <iomanip>
#include <map>
#include <sstream>
#include <vector>

namespace SolarSSL
{
struct FactoryPreset { juce::String category, name; std::map<juce::String, float> params; };

inline std::vector<FactoryPreset> getFactoryPresets()
{
    return {
        { "Default", "Init Console (Brown '02')", {{"eq_type",0.f},{"analog",1.f},{"width",-1.f}} },
        { "Default", "Init Console (Black '242')", {{"eq_type",1.f},{"analog",1.f},{"width",-1.f}} },
        { "Vocals", "Pop Lead Vocal (Air & Punch)", {{"eq_type",1.f},{"analog",1.f},{"mic_gain",12.f},{"hpf_freq",95.f},{"comp_ratio",4.f},{"comp_thresh",-10.f},{"comp_release",0.2f},{"hf_gain",3.5f},{"hf_freq",12000.f},{"hmf_gain",2.f},{"hmf_freq",3200.f},{"hmf_q",1.4f},{"lmf_gain",-2.5f},{"lmf_freq",320.f},{"lmf_q",1.8f},{"dyn_post_eq",1.f}} },
        { "Vocals", "Warm Rock Vocal (Brown Grit)", {{"eq_type",0.f},{"analog",1.f},{"mic_gain",22.f},{"hpf_freq",80.f},{"comp_ratio",6.f},{"comp_thresh",-12.f},{"comp_release",0.35f},{"comp_fatk",1.f},{"hf_gain",2.5f},{"hf_freq",8000.f},{"hmf_gain",3.f},{"hmf_freq",2200.f},{"lmf_gain",-2.f},{"lmf_freq",400.f}} },
        { "Vocals", "Vocal De-Ess Sidechain", {{"eq_type",1.f},{"eq_to_dyn_sc",1.f},{"comp_ratio",8.f},{"comp_thresh",-14.f},{"comp_release",0.1f},{"comp_fatk",1.f},{"hmf_gain",12.f},{"hmf_freq",6500.f},{"hmf_q",3.f}} },
        { "Drums", "Punchy Kick Drum", {{"eq_type",1.f},{"analog",1.f},{"mic_gain",14.f},{"hpf_freq",30.f},{"comp_ratio",4.f},{"comp_thresh",-8.f},{"comp_release",0.15f},{"exp_thresh",-18.f},{"exp_range",18.f},{"exp_release",0.2f},{"gate_mode",1.f},{"lf_gain",5.f},{"lf_freq",65.f},{"lf_bell",1.f},{"lmf_gain",-5.5f},{"lmf_freq",380.f},{"lmf_q",2.2f},{"hmf_gain",4.5f},{"hmf_freq",3800.f}} },
        { "Drums", "Crack Snare (Brown '02')", {{"eq_type",0.f},{"analog",1.f},{"mic_gain",18.f},{"hpf_freq",100.f},{"comp_ratio",5.f},{"comp_thresh",-9.f},{"comp_release",0.15f},{"exp_thresh",-22.f},{"exp_range",14.f},{"lf_gain",3.5f},{"lf_freq",190.f},{"lf_bell",1.f},{"lmf_gain",-3.f},{"lmf_freq",600.f},{"hf_gain",4.f},{"hf_freq",7500.f}} },
        { "Drums", "Drum Bus Glue & Sheen", {{"eq_type",0.f},{"analog",1.f},{"mic_gain",10.f},{"split_filters",1.f},{"hpf_freq",32.f},{"comp_ratio",3.f},{"comp_thresh",-6.f},{"comp_release",0.25f},{"lf_gain",2.f},{"lf_freq",80.f},{"hf_gain",2.5f},{"hf_freq",10000.f},{"extra_wide",1.f},{"width_filter",1.f}} },
        { "Bass", "Tight DI Bass (Black '242')", {{"eq_type",1.f},{"analog",1.f},{"mic_gain",16.f},{"hpf_freq",38.f},{"lpf_freq",6500.f},{"comp_ratio",5.f},{"comp_thresh",-11.f},{"comp_release",0.2f},{"comp_fatk",1.f},{"lf_gain",3.f},{"lf_freq",85.f},{"lmf_gain",2.5f},{"lmf_freq",750.f},{"width",0.f}} },
        { "Guitars", "Crunchy Electric Guitar", {{"eq_type",0.f},{"analog",1.f},{"mic_gain",15.f},{"hpf_freq",110.f},{"lpf_freq",9500.f},{"comp_ratio",2.5f},{"comp_thresh",-6.f},{"hmf_gain",3.5f},{"hmf_freq",2800.f},{"lmf_gain",-2.5f},{"lmf_freq",320.f}} },
        { "Master", "SSL 4000E Console Polish", {{"eq_type",1.f},{"analog",1.f},{"mic_gain",4.f},{"hpf_freq",24.f},{"comp_ratio",2.f},{"comp_thresh",-4.f},{"lf_gain",1.2f},{"lf_freq",60.f},{"hf_gain",1.5f},{"hf_freq",14000.f},{"extra_wide",1.f},{"width_filter",1.f}} }
    };
}

enum class KnobStyle { FlutedGold, WhiteComp, GreenExp, RedHF, GreenHMF, CyanLMF, BrownLF };

class SSLLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SSLLookAndFeel()
    {
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff231a0e));
        setColour(juce::ComboBox::textColourId, juce::Colour(0xfff5dfa4));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff9a7738));
        setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffe5c478));
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1e160c));
        setColour(juce::PopupMenu::textColourId, juce::Colour(0xfff5dfa4));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff8c6a2b));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    }

    static void drawEmbossedText(juce::Graphics& g, const juce::String& text, juce::Rectangle<int> r,
                                 float fontSize, juce::Colour faceCol = juce::Colour(0xfffaebbd),
                                 bool isDarkEngraved = false, juce::Justification just = juce::Justification::centred)
    {
        g.setFont(juce::FontOptions(fontSize, juce::Font::bold));
        g.setColour(juce::Colour(0xfffffce8).withAlpha(isDarkEngraved ? 0.65f : 0.72f));
        g.drawFittedText(text, r.translated(-1, -1), just, 1);
        g.setColour(juce::Colour(0xff1b0f03).withAlpha(isDarkEngraved ? 0.42f : 0.85f));
        g.drawFittedText(text, r.translated(1, 2), just, 1);
        g.setColour(juce::Colour(0xff3d2608).withAlpha(0.75f));
        g.drawFittedText(text, r.translated(1, 1), just, 1);
        g.setColour(faceCol);
        g.drawFittedText(text, r, just, 1);
    }

    static void drawBrushedGoldPanel(juce::Graphics& g, juce::Rectangle<float> b)
    {
        juce::ColourGradient grad(juce::Colour(0xffedd69c), b.getX(), b.getY(), juce::Colour(0xff5e4219), b.getRight(), b.getBottom(), false);
        grad.addColour(0.26, juce::Colour(0xffd4b16a));
        grad.addColour(0.58, juce::Colour(0xffa6803d));
        grad.addColour(0.84, juce::Colour(0xff775724));
        g.setGradientFill(grad);
        g.fillRect(b);

        juce::ColourGradient spotL(juce::Colour(0xfffff6ce).withAlpha(0.42f), b.getX() + 90.f, b.getY() + 40.f,
                                   juce::Colours::transparentBlack, b.getX() + 340.f, b.getY() + 340.f, true);
        g.setGradientFill(spotL);
        g.fillRect(b);

        g.setColour(juce::Colours::white.withAlpha(0.032f));
        for (float y = b.getY(); y < b.getBottom(); y += 3.0f) g.drawHorizontalLine(static_cast<int>(y), b.getX(), b.getRight());
        g.setColour(juce::Colours::black.withAlpha(0.035f));
        for (float y = b.getY() + 1.5f; y < b.getBottom(); y += 4.0f) g.drawHorizontalLine(static_cast<int>(y), b.getX(), b.getRight());

        g.setColour(juce::Colour(0xfffff8d6).withAlpha(0.52f));
        g.drawHorizontalLine(static_cast<int>(b.getY()), b.getX(), b.getRight());
        g.drawVerticalLine(static_cast<int>(b.getX()), b.getY(), b.getBottom());
        g.setColour(juce::Colour(0xff1c1104).withAlpha(0.75f));
        g.drawRect(b.reduced(0.5f), 1.5f);
    }

    static void drawJewelLED(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour onCol, bool isOn)
    {
        float cx = r.getCentreX(), cy = r.getCentreY(), rad = std::min(r.getWidth(), r.getHeight()) * 0.5f;
        g.setColour(juce::Colour(0xff33210a));
        g.fillEllipse(cx - rad - 1.8f, cy - rad - 1.8f, (rad + 1.8f) * 2.0f, (rad + 1.8f) * 2.0f);
        g.setColour(juce::Colour(0xfff5e0a8).withAlpha(0.38f));
        g.drawEllipse(cx - rad - 1.8f, cy - rad - 1.8f, (rad + 1.8f) * 2.0f, (rad + 1.8f) * 2.0f, 0.8f);
        if (isOn)
        {
            juce::ColourGradient glow(onCol.withAlpha(0.75f), cx, cy, onCol.withAlpha(0.0f), cx + rad * 2.7f, cy + rad * 2.7f, true);
            g.setGradientFill(glow);
            g.fillEllipse(cx - rad * 2.5f, cy - rad * 2.5f, rad * 5.0f, rad * 5.0f);
            juce::ColourGradient lens(onCol.brighter(0.9f), cx - rad * 0.3f, cy - rad * 0.3f, onCol.darker(0.35f), cx + rad, cy + rad, true);
            g.setGradientFill(lens);
        }
        else
        {
            juce::ColourGradient off(onCol.darker(0.78f), cx - rad * 0.25f, cy - rad * 0.25f, juce::Colour(0xff160f07), cx + rad, cy + rad, true);
            g.setGradientFill(off);
        }
        g.fillEllipse(cx - rad, cy - rad, rad * 2.0f, rad * 2.0f);
        g.setColour(juce::Colours::white.withAlpha(isOn ? 0.82f : 0.25f));
        g.fillEllipse(cx - rad * 0.45f, cy - rad * 0.5f, rad * 0.45f, rad * 0.35f);
    }

    static void drawSunburstLogo(juce::Graphics& g, juce::Rectangle<float> area)
    {
        auto c = juce::Point<float>(area.getCentreX(), area.getY() + area.getHeight() * 0.42f);
        float inR = area.getHeight() * 0.145f, outR = area.getHeight() * 0.28f;
        g.setColour(juce::Colour(0xff9b732e).withAlpha(0.78f));
        for (int i = 0; i < 12; ++i)
        {
            float a = static_cast<float>(i) * juce::MathConstants<float>::twoPi / 12.0f;
            juce::Path ray;
            ray.startNewSubPath(c.x + std::cos(a - 0.14f) * (inR + 2.0f), c.y + std::sin(a - 0.14f) * (inR + 2.0f));
            ray.lineTo(c.x + std::cos(a) * outR, c.y + std::sin(a) * outR);
            ray.lineTo(c.x + std::cos(a + 0.14f) * (inR + 2.0f), c.y + std::sin(a + 0.14f) * (inR + 2.0f));
            ray.closeSubPath();
            g.fillPath(ray);
        }
        g.setColour(juce::Colour(0xffb48a40).withAlpha(0.85f));
        g.fillEllipse(c.x - inR, c.y - inR, inR * 2.0f, inR * 2.0f);
        g.setColour(juce::Colour(0xfff6e5b3));
        g.fillEllipse(c.x - inR * 0.72f, c.y - inR * 0.72f, inR * 1.44f, inR * 1.44f);

        auto ta = area.withTrimmedTop(area.getHeight() * 0.28f).toNearestInt();
        g.setFont(juce::FontOptions(juce::Font::getDefaultSerifFontName(), 24.0f, juce::Font::plain));
        g.setColour(juce::Colour(0xfffff8db).withAlpha(0.85f));
        g.drawFittedText("SolarAudioSSL", ta.translated(-1, -1), juce::Justification::centred, 1);
        g.setColour(juce::Colour(0xff2d1b05).withAlpha(0.65f));
        g.drawFittedText("SolarAudioSSL", ta.translated(1, 2), juce::Justification::centred, 1);
        g.setColour(juce::Colour(0xff74521c));
        g.drawFittedText("SolarAudioSSL", ta, juce::Justification::centred, 1);
    }

    static void drawEV2Badge(juce::Graphics& g, juce::Rectangle<float> area)
    {
        auto r = area.toNearestInt();
        g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xfffff6d6).withAlpha(0.75f));
        g.drawFittedText("EV2", r.translated(-1, -1), juce::Justification::centredRight, 1);
        g.setColour(juce::Colour(0xff1a0e02).withAlpha(0.9f));
        g.drawFittedText("EV2", r.translated(2, 3), juce::Justification::centredRight, 1);
        g.setColour(juce::Colour(0xffe6c77e));
        g.drawFittedText("EV2", r, juce::Justification::centredRight, 1);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float startA, float endA, juce::Slider& s) override
    {
        float cx = x + w * 0.5f, cy = y + h * 0.5f, rad = std::min(w, h) * 0.36f;
        float angle = startA + pos * (endA - startA);
        auto style = static_cast<KnobStyle>(static_cast<int>(s.getProperties().getWithDefault("knobStyle", 0)));
        juce::String tickCsv = s.getProperties().getWithDefault("ticks", "").toString();

        juce::StringArray ticks;
        if (tickCsv.isNotEmpty()) ticks.addTokens(tickCsv, ",", "");
        int nTicks = ticks.isEmpty() ? 9 : ticks.size();
        g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        for (int i = 0; i < nTicks; ++i)
        {
            float t = (nTicks > 1) ? static_cast<float>(i) / static_cast<float>(nTicks - 1) : 0.5f;
            float ta = startA + t * (endA - startA) - juce::MathConstants<float>::halfPi;
            float r1 = rad + 2.2f, r2 = rad + 5.2f;
            g.setColour(juce::Colour(0xfff9e8b8).withAlpha(0.82f));
            g.drawLine(cx + std::cos(ta) * r1, cy + std::sin(ta) * r1, cx + std::cos(ta) * r2, cy + std::sin(ta) * r2, 1.0f);
            if (!ticks.isEmpty() && ticks[i].isNotEmpty())
            {
                float lx = cx + std::cos(ta) * (rad + 11.5f);
                float ly = cy + std::sin(ta) * (rad + 11.5f);
                juce::Rectangle<int> lr(static_cast<int>(lx - 13), static_cast<int>(ly - 5), 26, 10);
                g.setColour(juce::Colour(0xff231404).withAlpha(0.8f));
                g.drawFittedText(ticks[i], lr.translated(1, 1), juce::Justification::centred, 1);
                g.setColour(juce::Colour(0xfff8e7b8));
                g.drawFittedText(ticks[i], lr, juce::Justification::centred, 1);
            }
        }

        g.setColour(juce::Colours::black.withAlpha(0.32f));
        g.fillEllipse(cx - rad + 4.0f, cy - rad + 7.0f, rad * 2.0f, rad * 2.0f);
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillEllipse(cx - rad + 2.0f, cy - rad + 3.5f, rad * 2.0f, rad * 2.0f);

        juce::ColourGradient skirt(juce::Colour(0xfff7e3a6), cx - rad, cy - rad, juce::Colour(0xff4a3110), cx + rad, cy + rad, false);
        g.setGradientFill(skirt);
        g.fillEllipse(cx - rad, cy - rad, rad * 2.0f, rad * 2.0f);
        g.setColour(juce::Colour(0xff2e1c07));
        g.drawEllipse(cx - rad, cy - rad, rad * 2.0f, rad * 2.0f, 1.0f);

        for (int i = 0; i < 20; ++i)
        {
            float fa = i * juce::MathConstants<float>::twoPi / 20.0f + angle;
            g.setColour(juce::Colours::black.withAlpha(0.28f));
            g.drawLine(cx + std::cos(fa) * rad * 0.76f, cy + std::sin(fa) * rad * 0.76f, cx + std::cos(fa) * rad, cy + std::sin(fa) * rad, 1.25f);
        }

        float collarR = rad * 0.81f;
        g.setColour(juce::Colour(0xff231404).withAlpha(0.65f));
        g.fillEllipse(cx - collarR, cy - collarR, collarR * 2.0f, collarR * 2.0f);

        float capR = rad * 0.76f;
        juce::Colour cL(0xfff7e5b0), cD(0xff845f26), ptr(juce::Colours::white);
        switch (style)
        {
            case KnobStyle::FlutedGold: ptr = juce::Colour(0xff342008); break;
            case KnobStyle::WhiteComp:  cL = juce::Colour(0xfffcfaf2); cD = juce::Colour(0xffc0b8a8); ptr = juce::Colour(0xff2a1e10); break;
            case KnobStyle::GreenExp:
            case KnobStyle::GreenHMF:   cL = juce::Colour(0xff35b357); cD = juce::Colour(0xff125422); break;
            case KnobStyle::RedHF:      cL = juce::Colour(0xffe63535); cD = juce::Colour(0xff780f0f); break;
            case KnobStyle::CyanLMF:    cL = juce::Colour(0xff24a2bc); cD = juce::Colour(0xff0b4d5d); break;
            case KnobStyle::BrownLF:    cL = juce::Colour(0xff904a1d); cD = juce::Colour(0xff3b1806); break;
        }
        juce::ColourGradient cap(cL, cx - capR * 0.45f, cy - capR * 0.45f, cD, cx + capR * 0.75f, cy + capR * 0.75f, true);
        g.setGradientFill(cap);
        g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);

        g.setColour(juce::Colours::white.withAlpha(0.18f));
        g.fillEllipse(cx - capR * 0.58f, cy - capR * 0.68f, capR * 1.05f, capR * 0.58f);
        g.setColour(juce::Colour(0xfffffce8).withAlpha(0.42f));
        g.drawEllipse(cx - capR + 0.5f, cy - capR + 0.5f, (capR - 0.5f) * 2.0f, (capR - 0.5f) * 2.0f, 1.0f);

        float pa = angle - juce::MathConstants<float>::halfPi;
        g.setColour(ptr);
        g.drawLine(cx + std::cos(pa) * capR * 0.14f, cy + std::sin(pa) * capR * 0.14f, cx + std::cos(pa) * capR * 0.92f, cy + std::sin(pa) * capR * 0.92f, 2.5f);
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, const juce::Slider::SliderStyle, juce::Slider&) override
    {
        float cx = x + w * 0.5f;
        juce::Rectangle<float> tr(cx - 3.5f, y + 12.0f, 7.0f, h - 24.0f);
        g.setColour(juce::Colour(0xff0d0804)); g.fillRoundedRectangle(tr, 3.0f);
        g.setColour(juce::Colour(0xffdfbe76).withAlpha(0.5f)); g.drawRoundedRectangle(tr.expanded(1.0f), 3.0f, 1.0f);
        juce::Rectangle<float> cap(cx - 16.0f, pos - 24.0f, 32.0f, 48.0f);
        g.setColour(juce::Colours::black.withAlpha(0.62f)); g.fillRoundedRectangle(cap.translated(3.0f, 6.0f), 3.0f);
        juce::ColourGradient cg(juce::Colour(0xfff9e9ba), cap.getX(), cap.getY(), juce::Colour(0xff8e6a2e), cap.getX(), cap.getBottom(), false);
        cg.addColour(0.40, juce::Colour(0xffcaa45b)); cg.addColour(0.49, juce::Colour(0xff422c0d)); cg.addColour(0.51, juce::Colour(0xff1e1204)); cg.addColour(0.60, juce::Colour(0xfff2da9e));
        g.setGradientFill(cg); g.fillRoundedRectangle(cap, 3.0f);
        g.setColour(juce::Colour(0xff382309)); g.drawRoundedRectangle(cap, 3.0f, 1.2f);
        g.setColour(juce::Colours::white.withAlpha(0.55f)); g.drawHorizontalLine(static_cast<int>(cap.getY() + 1), cap.getX() + 2.f, cap.getRight() - 2.f);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hl, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced(1.5f);
        bool on = b.getToggleState(), tb = static_cast<bool>(b.getProperties().getWithDefault("toolbarBtn", false));
        if (tb)
        {
            auto bg = on ? juce::Colour(0xffb58d3f) : juce::Colour(0xff2a1f11);
            if (hl) bg = bg.brighter(0.15f);
            g.setColour(bg); g.fillRoundedRectangle(r, 3.0f);
            g.setColour(juce::Colour(0xffb99349)); g.drawRoundedRectangle(r, 3.0f, 1.0f);
            return;
        }
        if (!down) { g.setColour(juce::Colours::black.withAlpha(0.48f)); g.fillRoundedRectangle(r.translated(1.5f, 2.5f), 3.0f); }
        if (on) { g.setColour(juce::Colour(0xffffd86b).withAlpha(0.32f)); g.fillRoundedRectangle(r.expanded(2.0f), 4.0f); }
        juce::ColourGradient grad(on ? juce::Colour(0xfffffdef) : juce::Colour(0xffe8ca86), r.getX(), r.getY(),
                                  on ? juce::Colour(0xffe6c16c) : juce::Colour(0xff9a7634), r.getX(), r.getBottom(), false);
        g.setGradientFill(grad); g.fillRoundedRectangle(r, 3.0f);
        if (down) { g.setColour(juce::Colours::black.withAlpha(0.22f)); g.fillRoundedRectangle(r, 3.0f); }
        g.setColour(juce::Colour(0xff342108)); g.drawRoundedRectangle(r, 3.0f, 1.4f);
        g.setColour(juce::Colours::white.withAlpha(on ? 0.72f : 0.42f)); g.drawHorizontalLine(static_cast<int>(r.getY() + 1), r.getX() + 2.f, r.getRight() - 2.f);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        bool tb = static_cast<bool>(b.getProperties().getWithDefault("toolbarBtn", false));
        if (b.getProperties().contains("speakerIcon"))
        {
            auto c = b.getLocalBounds().toFloat().getCentre();
            juce::Path spk;
            spk.addRectangle(c.x - 8.f, c.y - 3.f, 4.f, 6.f);
            spk.startNewSubPath(c.x - 4.f, c.y - 3.f); spk.lineTo(c.x + 1.f, c.y - 7.f);
            spk.lineTo(c.x + 1.f, c.y + 7.f); spk.lineTo(c.x - 4.f, c.y + 3.f); spk.closeSubPath();
            g.setColour(juce::Colour(0xff2c1b07));
            g.fillPath(spk);
            juce::Path w1; w1.addCentredArc(c.x + 2.f, c.y, 4.f, 4.f, 0.f, -0.8f, 0.8f, true);
            juce::Path w2; w2.addCentredArc(c.x + 2.f, c.y, 7.f, 7.f, 0.f, -0.85f, 0.85f, true);
            g.strokePath(w1, juce::PathStrokeType(1.4f)); g.strokePath(w2, juce::PathStrokeType(1.4f));
            return;
        }
        g.setFont(juce::FontOptions(tb ? 11.5f : 10.5f, juce::Font::bold));
        g.setColour(tb ? (b.getToggleState() ? juce::Colour(0xff181006) : juce::Colour(0xfff4dfaa)) : juce::Colour(0xff2c1b07));
        g.drawFittedText(b.getButtonText(), b.getLocalBounds().reduced(2), juce::Justification::centred, 2);
    }
};
} // namespace SolarSSL

class SolarAudioSSLProcessor : public juce::AudioProcessor
{
public:
    SolarAudioSSLProcessor()
        : AudioProcessor(BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
              .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)),
          apvts(*this, &undoManager, "SolarAudioSSLState", createParameterLayout())
    {
        factoryPresets = SolarSSL::getFactoryPresets();
        stateSlotA = apvts.copyState();
        stateSlotB = apvts.copyState();
    }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
        auto f = [&](const char* id, const char* nm, float lo, float hi, float def, float sk = 1.0f) {
            p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(id, 1), nm, juce::NormalisableRange<float>(lo, hi, 0.01f, sk), def));
        };
        auto b = [&](const char* id, const char* nm, bool def) {
            p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID(id, 1), nm, def));
        };
        f("line_trim","Line Trim",-20,20,0); f("mic_gain","Mic Preamp",0,50,0);
        b("pad_20db","-20dB Pad",false); b("phase_invert","Phase Invert",false); b("analog","Analog",true);
        b("dyn_byp","Dyn Bypass",false); b("dyn_post_eq","CH OUT",false); b("split_filters","Split",false);
        b("eq_byp","EQ Bypass",false); b("eq_to_dyn_sc","EQ to Dyn SC",false); b("ext_sidechain","Ext SC",false); b("eq_listen","EQ Listen",false);
        f("comp_ratio","Comp Ratio",1,25,1,0.45f); f("comp_thresh","Comp Thresh",-20,10,10); f("comp_release","Comp Rel",0.1f,4.0f,0.3f,0.4f); b("comp_fatk","Comp F.ATK",false);
        f("exp_thresh","Exp Thresh",-30,10,-30); f("exp_range","Exp Range",0,40,0); f("exp_release","Exp Rel",0.1f,4.0f,0.3f,0.4f); b("exp_fatk","Exp F.ATK",false); b("gate_mode","Gate",false);
        b("eq_type","Black 242",false); f("hpf_freq","HPF",16,350,16,0.45f); f("lpf_freq","LPF",3000,22000,22000,0.45f);
        f("hf_gain","HF Gain",-18,18,0); f("hf_freq","HF Freq",1500,16000,8000,0.45f); b("hf_bell","HF Bell",false);
        f("hmf_gain","HMF Gain",-18,18,0); f("hmf_freq","HMF Freq",600,7000,2500,0.45f); f("hmf_q","HMF Q",0.1f,3.5f,1.5f,0.65f);
        f("lmf_gain","LMF Gain",-18,18,0); f("lmf_freq","LMF Freq",200,2500,800,0.45f); f("lmf_q","LMF Q",0.1f,3.5f,1.5f,0.65f);
        f("lf_gain","LF Gain",-18,18,0); f("lf_freq","LF Freq",30,450,120,0.45f); b("lf_bell","LF Bell",false);
        f("output_gain","Output",-48,12,0,1.6f); f("width","Width",-1,1,-1); b("extra_wide","Extra Wide",false); b("width_filter","Width Filter",false);
        return { p.begin(), p.end() };
    }

    void prepareToPlay(double sr, int) override { dsp.prepare(sr); resetPeakHold(); }
    void releaseResources() override { dsp.reset(); }
    bool isBusesLayoutSupported(const BusesLayout& l) const override
    {
        auto o = l.getMainOutputChannelSet(), i = l.getMainInputChannelSet();
        if ((o != juce::AudioChannelSet::mono() && o != juce::AudioChannelSet::stereo()) || i != o) return false;
        auto sc = l.getChannelSet(true, 1);
        return sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo();
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
    {
        juce::ScopedNoDenormals noDenormals;
        for (const auto m : midi)
        {
            auto msg = m.getMessage();
            if (msg.isController())
            {
                int cc = msg.getControllerNumber();
                if (cc >= 0 && cc < 128)
                {
                    juce::String pid;
                    {
                        const juce::ScopedLock sl(midiLock);
                        if (midiLearnActive.load() && lastTouchedParam.isNotEmpty()) { midiCcMap[static_cast<size_t>(cc)] = lastTouchedParam; midiLearnActive.store(false); }
                        pid = midiCcMap[static_cast<size_t>(cc)];
                    }
                    if (pid.isNotEmpty()) if (auto* param = apvts.getParameter(pid)) param->setValueNotifyingHost(msg.getControllerValue() / 127.0f);
                }
            }
        }
        auto mainBus = getBusBuffer(buffer, true, 0), outBus = getBusBuffer(buffer, false, 0), scBus = getBusBuffer(buffer, true, 1);
        int n = mainBus.getNumSamples(), chs = mainBus.getNumChannels(), scChs = scBus.getNumChannels();
        if (n == 0 || chs == 0) return;

        auto val = [&](const char* id) { return apvts.getRawParameterValue(id)->load(); };
        SolarSSL::ChannelParams p;
        p.lineTrimDb = val("line_trim"); p.micGainDb = val("mic_gain"); p.pad20Db = val("pad_20db") > 0.5f;
        p.phaseInvert = val("phase_invert") > 0.5f; p.analogOn = val("analog") > 0.5f;
        p.dynBypass = val("dyn_byp") > 0.5f; p.dynPostEq = val("dyn_post_eq") > 0.5f; p.splitFilters = val("split_filters") > 0.5f;
        p.eqBypass = val("eq_byp") > 0.5f; p.eqToDynSC = val("eq_to_dyn_sc") > 0.5f; p.extSidechain = (val("ext_sidechain") > 0.5f) && (scChs > 0);
        p.eqListen = val("eq_listen") > 0.5f;
        p.compRatio = val("comp_ratio"); p.compThreshDb = val("comp_thresh"); p.compReleaseSec = val("comp_release"); p.compFastAtk = val("comp_fatk") > 0.5f;
        p.expThreshDb = val("exp_thresh"); p.expRangeDb = val("exp_range"); p.expReleaseSec = val("exp_release"); p.expFastAtk = val("exp_fatk") > 0.5f; p.gateMode = val("gate_mode") > 0.5f;
        p.blackEqMode = val("eq_type") > 0.5f; p.hpfFreqHz = val("hpf_freq"); p.lpfFreqHz = val("lpf_freq");
        p.hfGainDb = val("hf_gain"); p.hfFreqHz = val("hf_freq"); p.hfBell = val("hf_bell") > 0.5f;
        p.hmfGainDb = val("hmf_gain"); p.hmfFreqHz = val("hmf_freq"); p.hmfQ = val("hmf_q");
        p.lmfGainDb = val("lmf_gain"); p.lmfFreqHz = val("lmf_freq"); p.lmfQ = val("lmf_q");
        p.lfGainDb = val("lf_gain"); p.lfFreqHz = val("lf_freq"); p.lfBell = val("lf_bell") > 0.5f;
        p.outputDb = val("output_gain"); p.widthNorm = val("width"); p.extraWide = val("extra_wide") > 0.5f; p.widthFilter = val("width_filter") > 0.5f;

        dsp.updateFilters(p);
        SolarSSL::MeterSnapshot ms;
        float* cL = outBus.getWritePointer(0); float* cR = (chs > 1) ? outBus.getWritePointer(1) : nullptr;
        const float* sL = (scChs > 0) ? scBus.getReadPointer(0) : nullptr; const float* sR = (scChs > 1) ? scBus.getReadPointer(1) : sL;
        for (int i = 0; i < n; ++i)
        {
            float l = cL[i], r = cR ? cR[i] : l;
            dsp.processStereoSample(l, r, sL ? sL[i] : 0.f, sR ? sR[i] : 0.f, p, ms);
            cL[i] = l; if (cR) cR[i] = r;
        }
        inPeakL.store(ms.inputPeakL); inPeakR.store(ms.inputPeakR);
        outPeakL.store(ms.outputPeakL); outPeakR.store(ms.outputPeakR);
        compGrDb.store(ms.compGrDb); gateGrDb.store(ms.gateGrDb);
    }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return static_cast<int>(factoryPresets.size()); }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram(int idx) override { loadFactoryPreset(idx); }
    const juce::String getProgramName(int idx) override { return (idx >= 0 && idx < getNumPrograms()) ? (factoryPresets[static_cast<size_t>(idx)].category + " / " + factoryPresets[static_cast<size_t>(idx)].name) : juce::String(); }
    void changeProgramName(int, const juce::String&) override {}

    void loadFactoryPreset(int idx)
    {
        if (idx < 0 || idx >= getNumPrograms()) return;
        currentPreset = idx;
        undoManager.beginNewTransaction("Preset");
        for (auto* pm : getParameters()) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(pm)) rp->setValueNotifyingHost(rp->getDefaultValue());
        for (const auto& kv : factoryPresets[static_cast<size_t>(idx)].params) if (auto* pm = apvts.getParameter(kv.first)) pm->setValueNotifyingHost(pm->convertTo0to1(kv.second));
    }

    void saveUserPreset(const juce::File& f) { if (auto xml = apvts.copyState().createXml()) xml->writeTo(f); }
    void loadUserPreset(const juce::File& f) { if (auto xml = juce::XmlDocument::parse(f)) if (xml->hasTagName(apvts.state.getType())) apvts.replaceState(juce::ValueTree::fromXml(*xml)); }
    void selectSlotA() { if (!slotB) return; stateSlotB = apvts.copyState(); slotB = false; apvts.replaceState(stateSlotA.createCopy()); }
    void selectSlotB() { if (slotB) return; stateSlotA = apvts.copyState(); slotB = true; apvts.replaceState(stateSlotB.createCopy()); }
    void copyAtoB() { if (slotB) stateSlotA = apvts.copyState(); else stateSlotB = apvts.copyState(); }
    bool isSlotB() const noexcept { return slotB; }
    void setMidiLearn(bool on) noexcept { midiLearnActive.store(on); }
    bool isMidiLearn() const noexcept { return midiLearnActive.load(); }
    void setLastTouched(const juce::String& id) { const juce::ScopedLock sl(midiLock); lastTouchedParam = id; }
    void resetPeakHold() noexcept { inPeakL = inPeakR = outPeakL = outPeakR = compGrDb = gateGrDb = 0.0f; }

    void getStateInformation(juce::MemoryBlock& dest) override { if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, dest); }
    void setStateInformation(const void* data, int sz) override { if (auto xml = getXmlFromBinary(data, sz)) if (xml->hasTagName(apvts.state.getType())) apvts.replaceState(juce::ValueTree::fromXml(*xml)); }

    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> inPeakL{0}, inPeakR{0}, outPeakL{0}, outPeakR{0}, compGrDb{0}, gateGrDb{0};

private:
    SolarSSL::SSLChannelDSP dsp;
    std::vector<SolarSSL::FactoryPreset> factoryPresets;
    int currentPreset = 0;
    juce::ValueTree stateSlotA, stateSlotB;
    bool slotB = false;
    std::atomic<bool> midiLearnActive{false};
    juce::CriticalSection midiLock;
    juce::String lastTouchedParam;
    std::array<juce::String, 128> midiCcMap;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SolarAudioSSLProcessor)
};

class SolarAudioSSLEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SolarAudioSSLEditor(SolarAudioSSLProcessor& p) : AudioProcessorEditor(&p), proc(p)
    {
        setLookAndFeel(&laf);
        addAndMakeVisible(presetBox);
        for (int i = 0; i < proc.getNumPrograms(); ++i) presetBox.addItem(proc.getProgramName(i), i + 1);
        presetBox.setSelectedId(proc.getCurrentProgram() + 1, juce::dontSendNotification);
        presetBox.onChange = [this] { if (presetBox.getSelectedId() > 0) proc.loadFactoryPreset(presetBox.getSelectedId() - 1); };

        auto mkTb = [&](juce::TextButton& b, const juce::String& t) { addAndMakeVisible(b); b.setButtonText(t); b.getProperties().set("toolbarBtn", true); };
        auto mkBt = [&](juce::TextButton& b, const juce::String& t, bool tog = true) { addAndMakeVisible(b); b.setButtonText(t); b.setClickingTogglesState(tog); };
        auto mkKn = [&](juce::Slider& s, SolarSSL::KnobStyle st, const juce::String& pid, const juce::String& ticks = "") {
            addAndMakeVisible(s); s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            s.getProperties().set("knobStyle", static_cast<int>(st));
            if (ticks.isNotEmpty()) s.getProperties().set("ticks", ticks);
            s.onDragStart = [this, pid] { proc.setLastTouched(pid); };
        };

        mkTb(saveBtn, "SAVE"); saveBtn.onClick = [this] {
            fc = std::make_unique<juce::FileChooser>("Save Preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.sslev2");
            fc->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& c) { auto f = c.getResult(); if (f != juce::File()) proc.saveUserPreset(f.withFileExtension(".sslev2")); });
        };
        mkTb(loadBtn, "LOAD"); loadBtn.onClick = [this] {
            fc = std::make_unique<juce::FileChooser>("Load Preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.sslev2");
            fc->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& c) { auto f = c.getResult(); if (f.existsAsFile()) proc.loadUserPreset(f); });
        };
        mkTb(slotABtn, "A"); slotABtn.setToggleState(!proc.isSlotB(), juce::dontSendNotification);
        slotABtn.onClick = [this] { proc.selectSlotA(); slotABtn.setToggleState(true, juce::dontSendNotification); slotBBtn.setToggleState(false, juce::dontSendNotification); };
        mkTb(slotBBtn, "B"); slotBBtn.setToggleState(proc.isSlotB(), juce::dontSendNotification);
        slotBBtn.onClick = [this] { proc.selectSlotB(); slotABtn.setToggleState(false, juce::dontSendNotification); slotBBtn.setToggleState(true, juce::dontSendNotification); };
        mkTb(copyBtn, "A->B"); copyBtn.onClick = [this] { proc.copyAtoB(); };
        mkTb(undoBtn, "UNDO"); undoBtn.onClick = [this] { proc.undoManager.undo(); };
        mkTb(redoBtn, "REDO"); redoBtn.onClick = [this] { proc.undoManager.redo(); };
        mkTb(extScBtn, "EXT S/C"); extScBtn.setClickingTogglesState(true);
        mkTb(midiBtn, "MIDI"); midiBtn.setClickingTogglesState(true); midiBtn.onClick = [this] { proc.setMidiLearn(midiBtn.getToggleState()); };

        addAndMakeVisible(scaleBox);
        scaleBox.addItem("75%", 75); scaleBox.addItem("90%", 90); scaleBox.addItem("100%", 100); scaleBox.addItem("125%", 125); scaleBox.addItem("150%", 150);
        scaleBox.setSelectedId(100, juce::dontSendNotification);
        scaleBox.onChange = [this] { applyScale(static_cast<float>(scaleBox.getSelectedId()) / 100.0f); };

        mkKn(lineK, SolarSSL::KnobStyle::FlutedGold, "line_trim", "-20,,0,,+20");
        mkKn(micK, SolarSSL::KnobStyle::FlutedGold, "mic_gain", "0,,25,,50");
        mkBt(analogB, "ANALOG"); mkBt(padB, "-20dB"); mkBt(phaseB, juce::String::fromUTF8("\xC3\x98"));
        mkBt(dynBypB, "BYP"); mkBt(chOutB, "CH.\nOUT");
        mkKn(cRatioK, SolarSSL::KnobStyle::WhiteComp, "comp_ratio", "1,2,3,5,8,20,INF");
        mkKn(cThreshK, SolarSSL::KnobStyle::WhiteComp, "comp_thresh", "10,5,0,-5,-10,-15,-20");
        mkKn(cRelK, SolarSSL::KnobStyle::WhiteComp, "comp_release", ".1,.15,.2,.4,1,2,4"); mkBt(cFatkB, "F.ATK");
        mkKn(eThreshK, SolarSSL::KnobStyle::GreenExp, "exp_thresh", "-30,-20,-12,-4,+10");
        mkKn(eRangeK, SolarSSL::KnobStyle::GreenExp, "exp_range", "0,6,10,20,35,40");
        mkKn(eRelK, SolarSSL::KnobStyle::GreenExp, "exp_release", ".1,.15,.2,.4,1,2,4"); mkBt(gateB, "GATE"); mkBt(eFatkB, "F.ATK");
        mkKn(hpfK, SolarSSL::KnobStyle::FlutedGold, "hpf_freq", "OUT,20,70,120,200,300,350");
        mkKn(lpfK, SolarSSL::KnobStyle::FlutedGold, "lpf_freq", "OUT,12,8,5,4,3.5,3"); mkBt(splitB, "SPLIT");

        mkBt(brownB, "", false); brownB.onClick = [this] { if (auto* pm = proc.apvts.getParameter("eq_type")) pm->setValueNotifyingHost(0.0f); repaint(); };
        mkBt(blackB, "", false); blackB.onClick = [this] { if (auto* pm = proc.apvts.getParameter("eq_type")) pm->setValueNotifyingHost(1.0f); repaint(); };
        mkBt(soloB, ""); soloB.getProperties().set("speakerIcon", true);
        mkBt(hfBellB, "BELL");
        mkKn(hfGainK, SolarSSL::KnobStyle::RedHF, "hf_gain", "-15,-9,-3,0,3,9,+15");
        mkKn(hfFreqK, SolarSSL::KnobStyle::RedHF, "hf_freq", "1.5,2,5,8,10,14,16");
        mkKn(hmfGainK, SolarSSL::KnobStyle::GreenHMF, "hmf_gain", "-15,-9,-3,0,3,9,+15");
        mkKn(hmfFreqK, SolarSSL::KnobStyle::GreenHMF, "hmf_freq", ".6,.8,1.5,3,4.5,6,7");
        mkKn(hmfQK, SolarSSL::KnobStyle::GreenHMF, "hmf_q", "3,2,1.5,1,.5");
        mkBt(eqBypB, "BYP"); mkBt(dynScB, "DYN\nS.C");
        mkKn(lmfGainK, SolarSSL::KnobStyle::CyanLMF, "lmf_gain", "-15,-9,-3,0,3,9,+15");
        mkKn(lmfFreqK, SolarSSL::KnobStyle::CyanLMF, "lmf_freq", ".2,.3,.8,1,1.5,2,2.5");
        mkKn(lmfQK, SolarSSL::KnobStyle::CyanLMF, "lmf_q", "3,2,1.5,1,.5");
        mkBt(lfBellB, "BELL");
        mkKn(lfGainK, SolarSSL::KnobStyle::BrownLF, "lf_gain", "-15,-9,-3,0,3,9,+15");
        mkKn(lfFreqK, SolarSSL::KnobStyle::BrownLF, "lf_freq", "30,50,100,200,300,350,450");

        addAndMakeVisible(outFader); outFader.setSliderStyle(juce::Slider::LinearVertical); outFader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        mkBt(mInB, "IN", false); mkBt(mOutB, "OUT", false); mOutB.setToggleState(true, juce::dontSendNotification);
        mInB.onClick = [this] { showInMeter = true; mInB.setToggleState(true, juce::dontSendNotification); mOutB.setToggleState(false, juce::dontSendNotification); repaint(); };
        mOutB.onClick = [this] { showInMeter = false; mInB.setToggleState(false, juce::dontSendNotification); mOutB.setToggleState(true, juce::dontSendNotification); repaint(); };
        mkKn(widthK, SolarSSL::KnobStyle::FlutedGold, "width"); mkBt(xWideB, "EXTRA\nWIDE"); mkBt(wFiltB, "FILTER");

        auto sAtt = [&](const char* id, juce::Slider& s) { sAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, id, s)); };
        auto bAtt = [&](const char* id, juce::Button& b) { bAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, id, b)); };
        sAtt("line_trim", lineK); sAtt("mic_gain", micK); bAtt("analog", analogB); bAtt("pad_20db", padB); bAtt("phase_invert", phaseB);
        bAtt("dyn_byp", dynBypB); bAtt("dyn_post_eq", chOutB); bAtt("ext_sidechain", extScBtn); bAtt("eq_listen", soloB);
        sAtt("comp_ratio", cRatioK); sAtt("comp_thresh", cThreshK); sAtt("comp_release", cRelK); bAtt("comp_fatk", cFatkB);
        sAtt("exp_thresh", eThreshK); sAtt("exp_range", eRangeK); sAtt("exp_release", eRelK); bAtt("gate_mode", gateB); bAtt("exp_fatk", eFatkB);
        sAtt("hpf_freq", hpfK); sAtt("lpf_freq", lpfK); bAtt("split_filters", splitB);
        bAtt("hf_bell", hfBellB); sAtt("hf_gain", hfGainK); sAtt("hf_freq", hfFreqK);
        sAtt("hmf_gain", hmfGainK); sAtt("hmf_freq", hmfFreqK); sAtt("hmf_q", hmfQK); bAtt("eq_byp", eqBypB); bAtt("eq_to_dyn_sc", dynScB);
        sAtt("lmf_gain", lmfGainK); sAtt("lmf_freq", lmfFreqK); sAtt("lmf_q", lmfQK);
        bAtt("lf_bell", lfBellB); sAtt("lf_gain", lfGainK); sAtt("lf_freq", lfFreqK);
        sAtt("output_gain", outFader); sAtt("width", widthK); bAtt("extra_wide", xWideB); bAtt("width_filter", wFiltB);

        setResizable(true, true);
        setResizeLimits(360, 475, 1440, 1900);
        if (auto* c = getConstrainer())
             c->setFixedAspectRatio(720.0 / 950.0);
        setSize(720, 950);
        startTimerHz(30);
    }

    ~SolarAudioSSLEditor() override { stopTimer(); setLookAndFeel(nullptr); }

    void applyScale(float sc)
    {
        setSize(static_cast<int>(std::round(720.0f * sc)), static_cast<int>(std::round(950.0f * sc)));
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (meterRect.contains(e.getPosition())) { peakL = peakR = -48.0f; proc.resetPeakHold(); repaint(); }
    }

    void timerCallback() override
    {
        float rL = showInMeter ? proc.inPeakL.load() : proc.outPeakL.load();
        float rR = showInMeter ? proc.inPeakR.load() : proc.outPeakR.load();
        mL = std::max(rL, mL * 0.84f); mR = std::max(rR, mR * 0.84f);
        peakL = std::max(peakL, SolarSSL::gainToDb(mL)); peakR = std::max(peakR, SolarSSL::gainToDb(mR));
        cGr = std::max(proc.compGrDb.load(), cGr * 0.82f); gGr = std::max(proc.gateGrDb.load(), gGr * 0.82f);
        bool blk = proc.apvts.getRawParameterValue("eq_type")->load() > 0.5f;
        brownB.setToggleState(!blk, juce::dontSendNotification); blackB.setToggleState(blk, juce::dontSendNotification);
        midiBtn.setToggleState(proc.isMidiLearn(), juce::dontSendNotification);
        repaint();
    }

    void resized() override
    {
        const float sc = static_cast<float>(getWidth()) / 720.0f;
        auto sb = [&](juce::Component& comp, int bx, int by, int bw, int bh) { comp.setBounds(static_cast<int>(std::round(bx * sc)), static_cast<int>(std::round(by * sc)), static_cast<int>(std::round(bw * sc)), static_cast<int>(std::round(bh * sc))); };
        sb(presetBox, 8,8,210,26); sb(saveBtn, 224,8,46,26); sb(loadBtn, 274,8,46,26);
        sb(slotABtn, 330,8,28,26); sb(slotBBtn, 361,8,28,26); sb(copyBtn, 392,8,42,26);
        sb(undoBtn, 444,8,44,26); sb(redoBtn, 491,8,44,26); sb(extScBtn, 544,8,56,26);
        sb(midiBtn, 604,8,42,26); sb(scaleBox, 650,8,62,26);
        const int y = 42;
        sb(lineK, 14,y+10,88,88); sb(micK, 14,y+114,88,88);
        sb(analogB, 138,y+18,68,31); sb(padB, 138,y+62,68,31); sb(phaseB, 138,y+106,68,31);
        sb(dynBypB, 130,y+192,36,52); sb(chOutB, 170,y+192,36,52);
        sb(cRatioK, 14,y+236,88,88); sb(cThreshK, 124,y+304,88,88); sb(cRelK, 14,y+364,88,88); sb(cFatkB, 58,y+462,46,18);
        sb(eThreshK, 124,y+426,88,88); sb(eRangeK, 14,y+482,88,88); sb(gateB, 138,y+530,66,30); sb(eRelK, 14,y+602,88,88); sb(eFatkB, 58,y+698,46,18);
        sb(hpfK, 16,y+742,88,88); sb(lpfK, 126,y+798,88,88); sb(splitB, 26,y+860,64,31);

        sb(brownB, 326,y+30,33,58); sb(blackB, 364,y+30,33,58); sb(soloB, 388,y+124,66,31);
        sb(hfBellB, 388,y+180,66,31); sb(hfGainK, 262,y+170,88,88); sb(hfFreqK, 376,y+238,88,88);
        sb(hmfGainK, 262,y+306,88,88); sb(hmfFreqK, 376,y+368,88,88); sb(hmfQK, 262,y+432,88,88);
        sb(eqBypB, 382,y+494,36,52); sb(dynScB, 422,y+494,36,52);
        sb(lmfGainK, 262,y+540,88,88); sb(lmfFreqK, 376,y+600,88,88); sb(lmfQK, 262,y+668,88,88);
        sb(lfGainK, 262,y+798,88,88); sb(lfFreqK, 376,y+736,88,88); sb(lfBellB, 388,y+860,66,31);

        sb(outFader, 526,y+210,44,466); sb(mInB, 620,y+152,39,30); sb(mOutB, 661,y+152,39,30);
        meterRect = { static_cast<int>(std::round(610 * sc)), static_cast<int>(std::round((y + 210) * sc)), static_cast<int>(std::round(102 * sc)), static_cast<int>(std::round(466 * sc)) };
        sb(widthK, 510,y+742,86,86); sb(xWideB, 632,y+741,66,34); sb(wFiltB, 632,y+789,66,34);
    }

    void paint(juce::Graphics& g) override
    {
        const float sc = static_cast<float>(getWidth()) / 720.0f;
        g.addTransform(juce::AffineTransform::scale(sc));
        const int y = 42;
        g.setColour(juce::Colour(0xff181108)); g.fillRect(0, 0, 720, y);
        g.setColour(juce::Colour(0xffa9823a)); g.drawHorizontalLine(y - 1, 0.0f, 720.0f);
        SolarSSL::SSLLookAndFeel::drawBrushedGoldPanel(g, {0.0f, static_cast<float>(y), 720.0f, 908.0f});

        for (float gx : {240.0f, 484.0f}) {
            g.setColour(juce::Colour(0xff2a1a06)); g.fillRect(gx - 1.5f, static_cast<float>(y), 2.5f, 908.0f);
            g.setColour(juce::Colour(0xfff9eab9).withAlpha(0.55f)); g.drawVerticalLine(static_cast<int>(gx + 1.5f), static_cast<float>(y), 950.0f);
        }

        g.setColour(juce::Colour(0xfffae8b6).withAlpha(0.58f));
        g.drawHorizontalLine(y + 226, 8.f, 112.f);
        juce::Path pL; pL.startNewSubPath(8.f, y + 480.f); pL.lineTo(96.f, y + 480.f); pL.lineTo(128.f, y + 424.f); pL.lineTo(232.f, y + 424.f);
        g.strokePath(pL, juce::PathStrokeType(1.1f));
        g.drawHorizontalLine(y + 734, 8.f, 232.f);
        juce::Path pM1; pM1.startNewSubPath(248.f, y + 282.f); pM1.lineTo(336.f, y + 282.f); pM1.lineTo(372.f, y + 342.f); pM1.lineTo(476.f, y + 342.f);
        g.strokePath(pM1, juce::PathStrokeType(1.1f));
        juce::Path pM2; pM2.startNewSubPath(248.f, y + 528.f); pM2.lineTo(336.f, y + 528.f); pM2.lineTo(372.f, y + 586.f); pM2.lineTo(476.f, y + 586.f);
        g.strokePath(pM2, juce::PathStrokeType(1.1f));
        g.drawHorizontalLine(y + 782, 248.f, 476.f);

        g.setColour(juce::Colour(0xff2e1c07)); g.drawHorizontalLine(y + 116, 486.0f, 720.0f);
        g.setColour(juce::Colour(0xfff9eab9).withAlpha(0.5f)); g.drawHorizontalLine(y + 117, 486.0f, 720.0f);
        SolarSSL::SSLLookAndFeel::drawSunburstLogo(g, {492.0f, static_cast<float>(y + 10), 220.0f, 98.0f});

        auto lbl = [&](const juce::String& t, int x, int ly, int w, int h, float sz = 11.0f, bool dark = false) {
            SolarSSL::SSLLookAndFeel::drawEmbossedText(g, t, {x, ly, w, h}, sz, dark ? juce::Colour(0xff4c320f) : juce::Colour(0xfffaebbd), dark);
        };
        auto fmt = [](float db) { if (db <= -47.9f) return juce::String("-inf"); std::ostringstream o; o << std::fixed << std::setprecision(1) << db; return juce::String(o.str()); };

        lbl("LINE",22,y+96,72,16,10.5f,true); lbl("MIC",22,y+200,72,16,10.5f,true);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {118.f,y+27.f,11.f,11.f}, juce::Colour(0xffffbe3b), analogB.getToggleState());
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {118.f,y+71.f,11.f,11.f}, juce::Colour(0xffffbe3b), padB.getToggleState());
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {118.f,y+115.f,11.f,11.f}, juce::Colour(0xffe6392e), phaseB.getToggleState());

        g.setColour(juce::Colour(0xfff5e2b1).withAlpha(0.65f)); g.drawRoundedRectangle(122.f,y+174.f,92.f,102.f,3.f,1.2f);
        lbl("DYN TO",136,y+167,64,15,10.5f);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {142.f,y+251.f,11.f,11.f}, juce::Colour(0xffe6392e), dynBypB.getToggleState());
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {182.f,y+251.f,11.f,11.f}, juce::Colour(0xffffbe3b), chOutB.getToggleState());

        lbl("RATIO",22,y+322,72,15,10.f); lbl("COMPRESS",12,y+346,96,18,13.f); lbl("THRESHOLD",122,y+391,92,15,10.f); lbl("RELEASE",22,y+449,72,15,10.f);
        lbl("THRESHOLD",122,y+511,92,15,10.f); lbl("RANGE",22,y+567,72,15,10.f); lbl("EXPAND",14,y+592,88,18,13.f); lbl("RELEASE",22,y+686,72,15,10.f);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {118.f,y+539.f,11.f,11.f}, juce::Colour(0xffffbe3b), gateB.getToggleState());

        juce::Rectangle<float> gr(130.f, y+576.f, 78.f, 152.f);
        g.setColour(juce::Colour(0xff24180a).withAlpha(0.84f)); g.fillRoundedRectangle(gr, 4.f);
        g.setColour(juce::Colour(0xffefd79d).withAlpha(0.65f)); g.drawRoundedRectangle(gr, 4.f, 1.2f);
        const float th[5] = {20.f, 14.f, 10.f, 6.f, 3.f};
        for (int i = 0; i < 5; ++i) {
            float ly = gr.getY() + 14.f + i * 28.f;
            SolarSSL::SSLLookAndFeel::drawJewelLED(g, {gr.getX()+10.f,ly,12.f,12.f}, juce::Colour(0xff4cd94c), gGr >= th[i]);
            lbl(juce::String(static_cast<int>(th[i])), static_cast<int>(gr.getX()+26), static_cast<int>(ly-1), 24, 15, 10.f);
            SolarSSL::SSLLookAndFeel::drawJewelLED(g, {gr.getRight()-22.f,ly,12.f,12.f}, i <= 1 ? juce::Colour(0xffe6392e) : juce::Colour(0xffffcc33), cGr >= th[i]);
        }

        lbl("FILTERS",130,y+766,82,18,12.5f); lbl("Hz",44,y+830,36,15,10.f); lbl("kHz",152,y+885,36,15,10.f);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {20.f,y+831.f,10.f,10.f}, juce::Colour(0xff5ce640), proc.apvts.getRawParameterValue("hpf_freq")->load() > 17.f);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {130.f,y+886.f,10.f,10.f}, juce::Colour(0xff5ce640), proc.apvts.getRawParameterValue("lpf_freq")->load() < 21800.f);

        g.setColour(juce::Colour(0xff5a3f17).withAlpha(0.65f)); g.drawRoundedRectangle(254.f,y+14.f,216.f,90.f,3.f,1.2f);
        lbl("EQ TYPE",318,y+6,88,16,12.f,true);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {335.f,y+64.f,14.f,14.f}, juce::Colour(0xffc96b1f), brownB.getToggleState());
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {373.f,y+64.f,14.f,14.f}, juce::Colour(0xff201a14), true);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {260.f,y+133.f,11.f,11.f}, juce::Colour(0xffe6392e), !eqBypB.getToggleState());
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {368.f,y+133.f,11.f,11.f}, juce::Colour(0xffffbe3b), soloB.getToggleState());
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {368.f,y+189.f,11.f,11.f}, juce::Colour(0xffffbe3b), hfBellB.getToggleState());

        lbl("HF",342,y+256,36,18,13.f); lbl("dB",288,y+256,36,14,10.f); lbl("kHz",402,y+325,38,14,10.f);
        lbl("HMF",322,y+418,46,18,13.f); lbl("dB",288,y+390,36,14,10.f); lbl("kHz",402,y+454,38,14,10.f); lbl("Q",288,y+514,36,14,10.f);
        g.setColour(juce::Colour(0xfff5e2b1).withAlpha(0.65f)); g.drawRoundedRectangle(374.f,y+478.f,92.f,102.f,3.f,1.2f);
        lbl("EQ TO",388,y+471,64,15,10.5f);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {394.f,y+555.f,11.f,11.f}, juce::Colour(0xffe6392e), eqBypB.getToggleState());
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {434.f,y+555.f,11.f,11.f}, juce::Colour(0xffffbe3b), dynScB.getToggleState());
        lbl("LMF",322,y+652,46,18,13.f); lbl("dB",288,y+623,36,14,10.f); lbl("kHz",402,y+687,38,14,10.f); lbl("Q",288,y+753,36,14,10.f);
        lbl("LF",342,y+812,36,18,13.f); lbl("Hz",402,y+824,38,14,10.f); lbl("dB",288,y+886,36,14,10.f);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {368.f,y+870.f,11.f,11.f}, juce::Colour(0xffffbe3b), lfBellB.getToggleState());

        lbl("OUTPUT",510,y+162,76,18,13.5f); lbl("METER",620,y+130,82,18,13.f);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {634.f,y+188.f,11.f,11.f}, juce::Colour(0xffffbe3b), showInMeter);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {675.f,y+188.f,11.f,11.f}, juce::Colour(0xffffbe3b), !showInMeter);
        const char* ft[11] = {"12","6","0","6","12","16","24","30","36","42","48"};
        for (int i = 0; i < 11; ++i) { lbl(ft[i],498,y+228+i*41,24,14,9.5f); lbl(ft[i],572,y+228+i*41,24,14,9.5f); }

        auto drawReadoutBox = [&](const juce::String& v, int rx) {
            juce::Rectangle<float> rb(static_cast<float>(rx), static_cast<float>(y + 691), 46.f, 19.f);
            g.setColour(juce::Colour(0xff473011)); g.fillRoundedRectangle(rb, 2.f);
            g.setColour(juce::Colour(0xfff5e2b0).withAlpha(0.32f)); g.drawRoundedRectangle(rb, 2.f, 0.8f);
            lbl(v, rx, y + 691, 46, 19, 12.f);
        };
        drawReadoutBox(fmt(proc.apvts.getRawParameterValue("output_gain")->load()), 525);

        auto bar = [&](juce::Rectangle<float> b, float lin) {
            g.setColour(juce::Colour(0xff140d06)); g.fillRoundedRectangle(b, 2.f);
            g.setColour(juce::Colour(0xffd7b66d).withAlpha(0.55f)); g.drawRoundedRectangle(b, 2.f, 1.f);
            float fh = b.getHeight() * ((SolarSSL::clampf(SolarSSL::gainToDb(lin), -48.f, 6.f) + 48.f) / 54.f);
            if (fh > 1.f) {
                auto fr = b.withTop(b.getBottom() - fh).reduced(1.5f, 1.f);
                juce::ColourGradient mg(juce::Colour(0xffffe066), fr.getX(), fr.getY(), juce::Colour(0xffb53d05), fr.getX(), fr.getBottom(), false);
                mg.addColour(0.35, juce::Colour(0xffffad26)); g.setGradientFill(mg); g.fillRect(fr);
            }
            g.setColour(juce::Colour(0xff140d06).withAlpha(0.36f));
            for (float sy = b.getY() + 4.f; sy < b.getBottom(); sy += 6.f) g.drawHorizontalLine(static_cast<int>(sy), b.getX() + 1.f, b.getRight() - 1.f);
        };
        bar({610.f,y+214.f,19.f,462.f}, mL); bar({688.f,y+214.f,19.f,462.f}, mR);
        const char* ms[12] = {"16","12","8","4","0","4","8","12","16","20","26","36"};
        for (int i = 0; i < 12; ++i) lbl(ms[i], 642, y+222+i*38, 34, 14, 9.5f);
        drawReadoutBox(fmt(peakL), 600); drawReadoutBox(fmt(peakR), 670);

        g.setColour(juce::Colour(0xff2e1c07)); g.drawHorizontalLine(y + 724, 486.f, 720.f);
        g.setColour(juce::Colour(0xfff9eab9).withAlpha(0.45f)); g.drawHorizontalLine(y + 725, 486.f, 720.f);
        lbl("MONO",522,y+734,62,15,10.f); lbl("STNDRD",492,y+824,54,15,10.f); lbl("REV",562,y+824,36,15,10.f); lbl("WIDTH",630,y+832,70,16,12.5f);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {612.f,y+753.f,11.f,11.f}, juce::Colour(0xffffbe3b), xWideB.getToggleState());
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {612.f,y+801.f,11.f,11.f}, juce::Colour(0xffffbe3b), wFiltB.getToggleState());
        SolarSSL::SSLLookAndFeel::drawEV2Badge(g, {606.f,y+868.f,98.f,30.f});
    }

private:
    SolarAudioSSLProcessor& proc;
    SolarSSL::SSLLookAndFeel laf;
    juce::ComboBox presetBox, scaleBox;
    juce::TextButton saveBtn, loadBtn, slotABtn, slotBBtn, copyBtn, undoBtn, redoBtn, extScBtn, midiBtn;
    juce::Slider lineK, micK, cRatioK, cThreshK, cRelK, eThreshK, eRangeK, eRelK, hpfK, lpfK;
    juce::TextButton analogB, padB, phaseB, dynBypB, chOutB, cFatkB, gateB, eFatkB, splitB;
    juce::TextButton brownB, blackB, soloB, hfBellB, eqBypB, dynScB, lfBellB;
    juce::Slider hfGainK, hfFreqK, hmfGainK, hmfFreqK, hmfQK, lmfGainK, lmfFreqK, lmfQK, lfGainK, lfFreqK;
    juce::Slider outFader, widthK;
    juce::TextButton mInB, mOutB, xWideB, wFiltB;
    bool showInMeter = false;
    float mL = 0.f, mR = 0.f, peakL = -48.f, peakR = -48.f, cGr = 0.f, gGr = 0.f;
    juce::Rectangle<int> meterRect;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sAtts;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> bAtts;
    std::unique_ptr<juce::FileChooser> fc;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SolarAudioSSLEditor)
};

juce::AudioProcessorEditor* SolarAudioSSLProcessor::createEditor() { return new SolarAudioSSLEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SolarAudioSSLProcessor(); }
