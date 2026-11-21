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

    static void drawBrushedGoldPanel(juce::Graphics& g, juce::Rectangle<float> b)
    {
        juce::ColourGradient grad(juce::Colour(0xffebd398), b.getX(), b.getY(), juce::Colour(0xff785928), b.getRight(), b.getBottom(), false);
        grad.addColour(0.28, juce::Colour(0xffd0ad66));
        grad.addColour(0.65, juce::Colour(0xff9c783b));
        g.setGradientFill(grad);
        g.fillRect(b);
        g.setColour(juce::Colours::white.withAlpha(0.032f));
        for (float y = b.getY(); y < b.getBottom(); y += 3.0f) g.drawHorizontalLine(static_cast<int>(y), b.getX(), b.getRight());
        g.setColour(juce::Colours::black.withAlpha(0.035f));
        for (float y = b.getY() + 1.5f; y < b.getBottom(); y += 4.0f) g.drawHorizontalLine(static_cast<int>(y), b.getX(), b.getRight());
    }

    static void drawJewelLED(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour onCol, bool isOn)
    {
        float cx = r.getCentreX(), cy = r.getCentreY(), rad = std::min(r.getWidth(), r.getHeight()) * 0.5f;
        g.setColour(juce::Colour(0xff3d2a10));
        g.fillEllipse(cx - rad - 1.5f, cy - rad - 1.5f, (rad + 1.5f) * 2.0f, (rad + 1.5f) * 2.0f);
        if (isOn)
        {
            juce::ColourGradient glow(onCol.withAlpha(0.65f), cx, cy, onCol.withAlpha(0.0f), cx + rad * 2.4f, cy + rad * 2.4f, true);
            g.setGradientFill(glow);
            g.fillEllipse(cx - rad * 2.2f, cy - rad * 2.2f, rad * 4.4f, rad * 4.4f);
            juce::ColourGradient lens(onCol.brighter(0.85f), cx - rad * 0.3f, cy - rad * 0.3f, onCol.darker(0.35f), cx + rad, cy + rad, true);
            g.setGradientFill(lens);
        }
        else
        {
            juce::ColourGradient off(onCol.darker(0.75f), cx - rad * 0.25f, cy - rad * 0.25f, juce::Colour(0xff1c140b), cx + rad, cy + rad, true);
            g.setGradientFill(off);
        }
        g.fillEllipse(cx - rad, cy - rad, rad * 2.0f, rad * 2.0f);
        g.setColour(juce::Colours::white.withAlpha(isOn ? 0.78f : 0.22f));
        g.fillEllipse(cx - rad * 0.45f, cy - rad * 0.5f, rad * 0.45f, rad * 0.35f);
    }

    static void drawSunburstLogo(juce::Graphics& g, juce::Rectangle<float> area)
    {
        auto c = juce::Point<float>(area.getCentreX(), area.getY() + area.getHeight() * 0.34f);
        float inR = area.getHeight() * 0.14f, outR = area.getHeight() * 0.26f;
        g.setColour(juce::Colour(0xff9b7433));
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
        g.setColour(juce::Colour(0xffb48a40));
        g.fillEllipse(c.x - inR, c.y - inR, inR * 2.0f, inR * 2.0f);
        g.setColour(juce::Colour(0xfff6e5b3));
        g.fillEllipse(c.x - inR * 0.72f, c.y - inR * 0.72f, inR * 1.44f, inR * 1.44f);
        auto ta = area.withTrimmedTop(area.getHeight() * 0.48f);
        g.setFont(juce::FontOptions(juce::Font::getDefaultSerifFontName(), 22.0f, juce::Font::plain));
        g.setColour(juce::Colour(0xfff8eab9).withAlpha(0.6f));
        g.drawFittedText("SolarAudioSSL", ta.translated(0.0f, 1.0f).toNearestInt(), juce::Justification::centred, 1);
        g.setColour(juce::Colour(0xff7d5b24));
        g.drawFittedText("SolarAudioSSL", ta.toNearestInt(), juce::Justification::centred, 1);
    }

    static void drawEV2Badge(juce::Graphics& g, juce::Rectangle<float> area)
    {
        g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff2c1d0a));
        g.drawFittedText("EV2", area.translated(1.0f, 1.5f).toNearestInt(), juce::Justification::centredRight, 1);
        g.setColour(juce::Colour(0xffdfc07a));
        g.drawFittedText("EV2", area.toNearestInt(), juce::Justification::centredRight, 1);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float startA, float endA, juce::Slider& s) override
    {
        float cx = x + w * 0.5f, cy = y + h * 0.5f, rad = std::min(w, h) * 0.42f;
        float angle = startA + pos * (endA - startA);
        auto style = static_cast<KnobStyle>(static_cast<int>(s.getProperties().getWithDefault("knobStyle", 0)));

        g.setColour(juce::Colour(0xfff6e4b5).withAlpha(0.75f));
        for (int i = 0; i < 11; ++i)
        {
            float ta = startA + (i / 10.0f) * (endA - startA) - juce::MathConstants<float>::halfPi;
            float r1 = rad + 2.5f, r2 = rad + ((i == 0 || i == 5 || i == 10) ? 6.0f : 4.2f);
            g.drawLine(cx + std::cos(ta) * r1, cy + std::sin(ta) * r1, cx + std::cos(ta) * r2, cy + std::sin(ta) * r2, 1.1f);
        }
        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.fillEllipse(cx - rad + 2.0f, cy - rad + 3.5f, rad * 2.0f, rad * 2.0f);

        juce::ColourGradient skirt(juce::Colour(0xfff2da99), cx - rad, cy - rad, juce::Colour(0xff593f18), cx + rad, cy + rad, false);
        g.setGradientFill(skirt);
        g.fillEllipse(cx - rad, cy - rad, rad * 2.0f, rad * 2.0f);
        for (int i = 0; i < 20; ++i)
        {
            float fa = i * juce::MathConstants<float>::twoPi / 20.0f + angle;
            g.setColour(juce::Colours::black.withAlpha(0.22f));
            g.drawLine(cx + std::cos(fa) * rad * 0.78f, cy + std::sin(fa) * rad * 0.78f, cx + std::cos(fa) * rad, cy + std::sin(fa) * rad, 1.2f);
        }

        float capR = rad * 0.78f;
        juce::Colour cL(0xfff5e0a8), cD(0xff8a672d), ptr(juce::Colours::white);
        switch (style)
        {
            case KnobStyle::FlutedGold: ptr = juce::Colour(0xff3a260b); break;
            case KnobStyle::WhiteComp:  cL = juce::Colour(0xfffaf7ef); cD = juce::Colour(0xffc9c2b2); ptr = juce::Colour(0xff2c2214); break;
            case KnobStyle::GreenExp:
            case KnobStyle::GreenHMF:   cL = juce::Colour(0xff33ab54); cD = juce::Colour(0xff155c28); break;
            case KnobStyle::RedHF:      cL = juce::Colour(0xffe03131); cD = juce::Colour(0xff801212); break;
            case KnobStyle::CyanLMF:    cL = juce::Colour(0xff229cb5); cD = juce::Colour(0xff0d5363); break;
            case KnobStyle::BrownLF:    cL = juce::Colour(0xff8a461b); cD = juce::Colour(0xff421d08); break;
        }
        juce::ColourGradient cap(cL, cx - capR * 0.5f, cy - capR * 0.5f, cD, cx + capR * 0.7f, cy + capR * 0.7f, false);
        g.setGradientFill(cap);
        g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.28f));
        g.drawEllipse(cx - capR + 0.5f, cy - capR + 0.5f, (capR - 0.5f) * 2.0f, (capR - 0.5f) * 2.0f, 1.0f);

        float pa = angle - juce::MathConstants<float>::halfPi;
        g.setColour(ptr);
        g.drawLine(cx + std::cos(pa) * capR * 0.15f, cy + std::sin(pa) * capR * 0.15f, cx + std::cos(pa) * capR * 0.92f, cy + std::sin(pa) * capR * 0.92f, 2.4f);
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, const juce::Slider::SliderStyle, juce::Slider&) override
    {
        float cx = x + w * 0.5f;
        juce::Rectangle<float> tr(cx - 3.0f, y + 12.0f, 6.0f, h - 24.0f);
        g.setColour(juce::Colour(0xff100b06)); g.fillRoundedRectangle(tr, 3.0f);
        g.setColour(juce::Colour(0xffdfbe76).withAlpha(0.45f)); g.drawRoundedRectangle(tr.expanded(1.0f), 3.0f, 1.0f);
        juce::Rectangle<float> cap(cx - 14.0f, pos - 22.0f, 28.0f, 44.0f);
        g.setColour(juce::Colours::black.withAlpha(0.55f)); g.fillRoundedRectangle(cap.translated(2.0f, 4.0f), 3.0f);
        juce::ColourGradient cg(juce::Colour(0xfff7e5b5), cap.getX(), cap.getY(), juce::Colour(0xff8e6a2e), cap.getX(), cap.getBottom(), false);
        cg.addColour(0.42, juce::Colour(0xffcaa45b)); cg.addColour(0.50, juce::Colour(0xff523914)); cg.addColour(0.58, juce::Colour(0xfff0d89d));
        g.setGradientFill(cg); g.fillRoundedRectangle(cap, 3.0f);
        g.setColour(juce::Colour(0xff3c280c)); g.drawRoundedRectangle(cap, 3.0f, 1.1f);
        g.setColour(juce::Colour(0xff1f1407)); g.fillRect(cap.getX() + 1.0f, cap.getCentreY() - 1.0f, cap.getWidth() - 2.0f, 2.0f);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hl, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced(1.0f);
        bool on = b.getToggleState(), tb = static_cast<bool>(b.getProperties().getWithDefault("toolbarBtn", false));
        if (tb)
        {
            auto bg = on ? juce::Colour(0xffa98237) : juce::Colour(0xff2a1f11);
            if (hl) bg = bg.brighter(0.15f);
            g.setColour(bg); g.fillRoundedRectangle(r, 3.0f);
            g.setColour(juce::Colour(0xffb99349)); g.drawRoundedRectangle(r, 3.0f, 1.0f);
            return;
        }
        juce::ColourGradient grad(on ? juce::Colour(0xfffff9e2) : juce::Colour(0xffdec07e), r.getX(), r.getY(),
                                  on ? juce::Colour(0xffe7c270) : juce::Colour(0xff9c7838), r.getX(), r.getBottom(), false);
        g.setGradientFill(grad); g.fillRoundedRectangle(r, 3.0f);
        if (down) { g.setColour(juce::Colours::black.withAlpha(0.18f)); g.fillRoundedRectangle(r, 3.0f); }
        g.setColour(juce::Colour(0xff38250b)); g.drawRoundedRectangle(r, 3.0f, 1.3f);
        g.setColour(juce::Colours::white.withAlpha(on ? 0.55f : 0.25f)); g.drawRoundedRectangle(r.reduced(1.0f), 2.2f, 0.8f);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        bool tb = static_cast<bool>(b.getProperties().getWithDefault("toolbarBtn", false));
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
        auto mkKn = [&](juce::Slider& s, SolarSSL::KnobStyle st, const juce::String& pid) {
            addAndMakeVisible(s); s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            s.getProperties().set("knobStyle", static_cast<int>(st));
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
        scaleBox.addItem("75%", 75); scaleBox.addItem("100%", 100); scaleBox.addItem("125%", 125); scaleBox.addItem("150%", 150);
        scaleBox.setSelectedId(100, juce::dontSendNotification);
        scaleBox.onChange = [this] { float sc = scaleBox.getSelectedId() / 100.0f; setTransform(juce::AffineTransform::scale(sc)); setSize(static_cast<int>(720 * sc), static_cast<int>(950 * sc)); };

        mkKn(lineK, SolarSSL::KnobStyle::FlutedGold, "line_trim"); mkKn(micK, SolarSSL::KnobStyle::FlutedGold, "mic_gain");
        mkBt(analogB, "ANALOG"); mkBt(padB, "-20dB"); mkBt(phaseB, juce::String::fromUTF8("\xC3\x98"));
        mkBt(dynBypB, "BYP"); mkBt(chOutB, "CH.\nOUT");
        mkKn(cRatioK, SolarSSL::KnobStyle::WhiteComp, "comp_ratio"); mkKn(cThreshK, SolarSSL::KnobStyle::WhiteComp, "comp_thresh"); mkKn(cRelK, SolarSSL::KnobStyle::WhiteComp, "comp_release"); mkBt(cFatkB, "F.ATK");
        mkKn(eThreshK, SolarSSL::KnobStyle::GreenExp, "exp_thresh"); mkKn(eRangeK, SolarSSL::KnobStyle::GreenExp, "exp_range"); mkKn(eRelK, SolarSSL::KnobStyle::GreenExp, "exp_release"); mkBt(gateB, "GATE"); mkBt(eFatkB, "F.ATK");
        mkKn(hpfK, SolarSSL::KnobStyle::FlutedGold, "hpf_freq"); mkKn(lpfK, SolarSSL::KnobStyle::FlutedGold, "lpf_freq"); mkBt(splitB, "SPLIT");

        mkBt(brownB, "", false); brownB.onClick = [this] { if (auto* pm = proc.apvts.getParameter("eq_type")) pm->setValueNotifyingHost(0.0f); repaint(); };
        mkBt(blackB, "", false); blackB.onClick = [this] { if (auto* pm = proc.apvts.getParameter("eq_type")) pm->setValueNotifyingHost(1.0f); repaint(); };
        mkBt(soloB, "SOLO"); mkBt(hfBellB, "BELL"); mkKn(hfGainK, SolarSSL::KnobStyle::RedHF, "hf_gain"); mkKn(hfFreqK, SolarSSL::KnobStyle::RedHF, "hf_freq");
        mkKn(hmfGainK, SolarSSL::KnobStyle::GreenHMF, "hmf_gain"); mkKn(hmfFreqK, SolarSSL::KnobStyle::GreenHMF, "hmf_freq"); mkKn(hmfQK, SolarSSL::KnobStyle::GreenHMF, "hmf_q");
        mkBt(eqBypB, "BYP"); mkBt(dynScB, "DYN\nS.C");
        mkKn(lmfGainK, SolarSSL::KnobStyle::CyanLMF, "lmf_gain"); mkKn(lmfFreqK, SolarSSL::KnobStyle::CyanLMF, "lmf_freq"); mkKn(lmfQK, SolarSSL::KnobStyle::CyanLMF, "lmf_q");
        mkBt(lfBellB, "BELL"); mkKn(lfGainK, SolarSSL::KnobStyle::BrownLF, "lf_gain"); mkKn(lfFreqK, SolarSSL::KnobStyle::BrownLF, "lf_freq");

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

        setSize(720, 950);
        startTimerHz(30);
    }

    ~SolarAudioSSLEditor() override { stopTimer(); setLookAndFeel(nullptr); }

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
        presetBox.setBounds(8,8,210,26); saveBtn.setBounds(224,8,46,26); loadBtn.setBounds(274,8,46,26);
        slotABtn.setBounds(330,8,28,26); slotBBtn.setBounds(361,8,28,26); copyBtn.setBounds(392,8,42,26);
        undoBtn.setBounds(444,8,44,26); redoBtn.setBounds(491,8,44,26); extScBtn.setBounds(544,8,56,26);
        midiBtn.setBounds(604,8,42,26); scaleBox.setBounds(650,8,62,26);
        const int y = 42;
        lineK.setBounds(26,y+18,66,66); micK.setBounds(26,y+126,66,66);
        analogB.setBounds(140,y+18,66,30); padB.setBounds(140,y+62,66,30); phaseB.setBounds(140,y+106,66,30);
        dynBypB.setBounds(132,y+194,34,46); chOutB.setBounds(170,y+194,36,46);
        cRatioK.setBounds(26,y+250,68,68); cThreshK.setBounds(134,y+318,68,68); cRelK.setBounds(26,y+382,68,68); cFatkB.setBounds(58,y+462,46,18);
        eThreshK.setBounds(134,y+444,68,68); eRangeK.setBounds(26,y+498,68,68); gateB.setBounds(140,y+530,64,28); eRelK.setBounds(26,y+618,68,68); eFatkB.setBounds(58,y+698,46,18);
        hpfK.setBounds(28,y+754,68,68); lpfK.setBounds(136,y+812,68,68); splitB.setBounds(28,y+860,62,30);
        brownB.setBounds(324,y+32,34,56); blackB.setBounds(362,y+32,34,56); soloB.setBounds(388,y+126,64,30);
        hfBellB.setBounds(388,y+184,64,30); hfGainK.setBounds(274,y+186,68,68); hfFreqK.setBounds(386,y+256,68,68);
        hmfGainK.setBounds(274,y+322,68,68); hmfFreqK.setBounds(386,y+384,68,68); hmfQK.setBounds(274,y+448,68,68);
        eqBypB.setBounds(386,y+496,34,48); dynScB.setBounds(424,y+496,36,48);
        lmfGainK.setBounds(274,y+556,68,68); lmfFreqK.setBounds(386,y+616,68,68); lmfQK.setBounds(274,y+686,68,68);
        lfGainK.setBounds(274,y+816,68,68); lfFreqK.setBounds(386,y+754,68,68); lfBellB.setBounds(388,y+858,64,30);
        outFader.setBounds(526,y+210,44,466); mInB.setBounds(624,y+154,38,30); mOutB.setBounds(664,y+154,38,30);
        meterRect = {610,y+210,102,466};
        widthK.setBounds(520,y+754,66,66); xWideB.setBounds(634,y+742,62,32); wFiltB.setBounds(634,y+790,62,32);
    }

    void paint(juce::Graphics& g) override
    {
        const int y = 42;
        g.setColour(juce::Colour(0xff1a130a)); g.fillRect(0, 0, 720, y);
        g.setColour(juce::Colour(0xffa9823a)); g.drawHorizontalLine(y - 1, 0.0f, 720.0f);
        SolarSSL::SSLLookAndFeel::drawBrushedGoldPanel(g, {0.0f, static_cast<float>(y), 720.0f, 908.0f});

        for (float gx : {240.0f, 484.0f}) {
            g.setColour(juce::Colour(0xff38250b)); g.fillRect(gx - 1.5f, static_cast<float>(y), 2.5f, 908.0f);
            g.setColour(juce::Colour(0xfff7e6b7).withAlpha(0.55f)); g.drawVerticalLine(static_cast<int>(gx + 1.5f), static_cast<float>(y), 950.0f);
        }
        g.setColour(juce::Colour(0xff38250b)); g.drawHorizontalLine(y + 114, 486.0f, 720.0f);
        SolarSSL::SSLLookAndFeel::drawSunburstLogo(g, {494.0f, static_cast<float>(y + 8), 216.0f, 98.0f});

        auto lbl = [&](const juce::String& t, int x, int ly, int w, int h, float sz = 11.0f, juce::Colour c = juce::Colour(0xfff7e6b5)) {
            g.setFont(juce::FontOptions(sz, juce::Font::bold));
            g.setColour(juce::Colour(0xff3b270b).withAlpha(0.65f)); g.drawFittedText(t, x + 1, ly + 1, w, h, juce::Justification::centred, 1);
            g.setColour(c); g.drawFittedText(t, x, ly, w, h, juce::Justification::centred, 1);
        };
        auto fmt = [](float db) { if (db <= -47.9f) return juce::String("-inf"); std::ostringstream o; o << std::fixed << std::setprecision(1) << db; return juce::String(o.str()); };

        lbl("LINE",26,y+84,66,16,11.f,juce::Colour(0xff4a3210)); lbl("MIC",26,y+194,66,16,11.f,juce::Colour(0xff4a3210));
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {120.f,y+27.f,12.f,12.f}, juce::Colour(0xffffbe3b), analogB.getToggleState());
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {120.f,y+71.f,12.f,12.f}, juce::Colour(0xffffbe3b), padB.getToggleState());
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {120.f,y+115.f,12.f,12.f}, juce::Colour(0xffe6392e), phaseB.getToggleState());

        g.setColour(juce::Colour(0xfff5e2b1).withAlpha(0.65f)); g.drawRoundedRectangle(124.f,y+178.f,90.f,90.f,3.f,1.f);
        lbl("DYN TO",136,y+168,66,16,10.5f);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {143.f,y+247.f,11.f,11.f}, juce::Colour(0xffe6392e), dynBypB.getToggleState());
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {182.f,y+247.f,11.f,11.f}, juce::Colour(0xffffbe3b), chOutB.getToggleState());

        lbl("RATIO",26,y+320,68,16,10.5f); lbl("COMPRESS",16,y+348,96,18,12.5f); lbl("THRESHOLD",122,y+388,92,16,10.5f); lbl("RELEASE",24,y+448,72,16,10.5f);
        lbl("THRESHOLD",122,y+512,92,16,10.5f); lbl("RANGE",26,y+566,68,16,10.5f); lbl("EXPAND",18,y+592,86,18,12.5f); lbl("RELEASE",24,y+684,72,16,10.5f);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {121.f,y+538.f,12.f,12.f}, juce::Colour(0xffffbe3b), gateB.getToggleState());

        juce::Rectangle<float> gr(132.f, y+576.f, 76.f, 152.f);
        g.setColour(juce::Colour(0xff2c1f0e).withAlpha(0.75f)); g.fillRoundedRectangle(gr, 4.f);
        g.setColour(juce::Colour(0xffefd79d).withAlpha(0.65f)); g.drawRoundedRectangle(gr, 4.f, 1.f);
        const float th[5] = {20.f, 14.f, 10.f, 6.f, 3.f};
        for (int i = 0; i < 5; ++i) {
            float ly = gr.getY() + 14.f + i * 28.f;
            SolarSSL::SSLLookAndFeel::drawJewelLED(g, {gr.getX()+10.f,ly,13.f,13.f}, juce::Colour(0xff4cd94c), gGr >= th[i]);
            lbl(juce::String(static_cast<int>(th[i])), static_cast<int>(gr.getX()+26), static_cast<int>(ly-1), 24, 15, 10.f);
            SolarSSL::SSLLookAndFeel::drawJewelLED(g, {gr.getRight()-23.f,ly,13.f,13.f}, i <= 1 ? juce::Colour(0xffe6392e) : juce::Colour(0xffffcc33), cGr >= th[i]);
        }

        lbl("FILTERS",132,y+772,78,18,12.f); lbl("Hz",44,y+826,36,15,10.5f); lbl("kHz",152,y+882,36,15,10.5f);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {20.f,y+828.f,10.f,10.f}, juce::Colour(0xff5ce640), proc.apvts.getRawParameterValue("hpf_freq")->load() > 17.f);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {130.f,y+884.f,10.f,10.f}, juce::Colour(0xff5ce640), proc.apvts.getRawParameterValue("lpf_freq")->load() < 21800.f);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {12.f,y+870.f,10.f,10.f}, juce::Colour(0xffffbe3b), splitB.getToggleState());

        g.setColour(juce::Colour(0xff5a3f17).withAlpha(0.6f)); g.drawRoundedRectangle(254.f,y+14.f,214.f,92.f,3.f,1.f);
        lbl("EQ TYPE",320,y+6,82,16,11.5f,juce::Colour(0xff4a3210));
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {334.f,y+64.f,14.f,14.f}, juce::Colour(0xffc96b1f), brownB.getToggleState());
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {372.f,y+64.f,14.f,14.f}, juce::Colour(0xff201a14), true);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {260.f,y+134.f,11.f,11.f}, juce::Colour(0xffe6392e), !eqBypB.getToggleState());
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {370.f,y+134.f,11.f,11.f}, juce::Colour(0xffffbe3b), soloB.getToggleState());
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {370.f,y+193.f,11.f,11.f}, juce::Colour(0xffffbe3b), hfBellB.getToggleState());
        lbl("HF",344,y+256,36,18,12.5f); lbl("dB",292,y+252,32,15,10.f); lbl("kHz",404,y+324,34,15,10.f);
        lbl("HMF",336,y+410,44,18,12.5f); lbl("dB",292,y+388,32,15,10.f); lbl("kHz",404,y+452,34,15,10.f); lbl("Q",292,y+514,32,15,10.f);
        g.setColour(juce::Colour(0xfff5e2b1).withAlpha(0.65f)); g.drawRoundedRectangle(378.f,y+480.f,90.f,92.f,3.f,1.f);
        lbl("EQ TO",390,y+470,66,16,10.5f);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {397.f,y+551.f,11.f,11.f}, juce::Colour(0xffe6392e), eqBypB.getToggleState());
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {436.f,y+551.f,11.f,11.f}, juce::Colour(0xffffbe3b), dynScB.getToggleState());
        lbl("LMF",336,y+644,44,18,12.5f); lbl("dB",292,y+622,32,15,10.f); lbl("kHz",404,y+684,34,15,10.f); lbl("Q",292,y+752,32,15,10.f);
        lbl("LF",344,y+812,36,18,12.5f); lbl("Hz",404,y+822,34,15,10.f); lbl("dB",292,y+882,32,15,10.f);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {370.f,y+867.f,11.f,11.f}, juce::Colour(0xffffbe3b), lfBellB.getToggleState());

        lbl("OUTPUT",510,y+162,76,18,13.f); lbl("METER",622,y+130,80,18,12.5f);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {637.f,y+190.f,11.f,11.f}, juce::Colour(0xffffbe3b), showInMeter);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {677.f,y+190.f,11.f,11.f}, juce::Colour(0xffffbe3b), !showInMeter);
        const char* ft[11] = {"12","6","0","6","12","16","24","30","36","42","48"};
        for (int i = 0; i < 11; ++i) { lbl(ft[i],498,y+228+i*42,24,14,9.5f); lbl(ft[i],572,y+228+i*42,24,14,9.5f); }
        lbl(fmt(proc.apvts.getRawParameterValue("output_gain")->load()), 520, y+694, 56, 18, 12.f);

        auto bar = [&](juce::Rectangle<float> b, float lin) {
            g.setColour(juce::Colour(0xff140d06)); g.fillRoundedRectangle(b, 2.f);
            g.setColour(juce::Colour(0xffd7b66d).withAlpha(0.5f)); g.drawRoundedRectangle(b, 2.f, 1.f);
            float fh = b.getHeight() * ((SolarSSL::clampf(SolarSSL::gainToDb(lin), -48.f, 6.f) + 48.f) / 54.f);
            if (fh > 1.f) {
                auto fr = b.withTop(b.getBottom() - fh).reduced(1.5f, 1.f);
                juce::ColourGradient mg(juce::Colour(0xffffd447), fr.getX(), fr.getY(), juce::Colour(0xffcc4e08), fr.getX(), fr.getBottom(), false);
                mg.addColour(0.35, juce::Colour(0xffff9a1f)); g.setGradientFill(mg); g.fillRect(fr);
            }
        };
        bar({612.f,y+216.f,18.f,456.f}, mL); bar({690.f,y+216.f,18.f,456.f}, mR);
        const char* ms[12] = {"16","12","8","4","0","4","8","12","16","20","26","36"};
        for (int i = 0; i < 12; ++i) lbl(ms[i], 644, y+222+i*39, 32, 14, 9.5f);
        lbl(fmt(peakL), 602, y+694, 48, 18, 11.5f); lbl(fmt(peakR), 672, y+694, 48, 18, 11.5f);

        g.setColour(juce::Colour(0xff38250b)); g.drawHorizontalLine(y + 726, 486.f, 720.f);
        lbl("MONO",522,y+736,62,15,10.f); lbl("STNDRD",494,y+822,54,15,10.f); lbl("REV",564,y+822,36,15,10.f); lbl("WIDTH",630,y+830,70,16,11.5f);
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {614.f,y+752.f,12.f,12.f}, juce::Colour(0xffffbe3b), xWideB.getToggleState());
        SolarSSL::SSLLookAndFeel::drawJewelLED(g, {614.f,y+800.f,12.f,12.f}, juce::Colour(0xffffbe3b), wFiltB.getToggleState());
        SolarSSL::SSLLookAndFeel::drawEV2Badge(g, {606.f,y+868.f,96.f,30.f});
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
