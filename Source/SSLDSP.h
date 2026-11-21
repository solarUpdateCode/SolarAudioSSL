#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace SolarSSL
{
constexpr double kPi = 3.14159265358979323846;

inline float dbToGain(float db) noexcept { return std::pow(10.0f, db * 0.05f); }
inline float gainToDb(float gain) noexcept { return 20.0f * std::log10(std::max(gain, 1.0e-6f)); }
inline float clampf(float v, float lo, float hi) noexcept { return std::max(lo, std::min(hi, v)); }

struct Biquad
{
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0, z1 = 0.0, z2 = 0.0;
    void reset() noexcept { z1 = 0.0; z2 = 0.0; }
    void setBypass() noexcept { b0 = 1.0; b1 = b2 = a1 = a2 = 0.0; }

    inline double process(double x) noexcept
    {
        double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        if (std::abs(z1) < 1.0e-20) z1 = 0.0;
        if (std::abs(z2) < 1.0e-20) z2 = 0.0;
        return y;
    }

    void setOnePoleHPF(double sr, double freq) noexcept
    {
        freq = std::clamp(freq, 10.0, sr * 0.45);
        double w = std::tan(kPi * freq / sr);
        double n = 1.0 / (1.0 + w);
        b0 = n; b1 = -n; b2 = 0.0; a1 = (w - 1.0) * n; a2 = 0.0;
    }

    void setHPF2(double sr, double freq, double q = 0.70710678) noexcept
    {
        freq = std::clamp(freq, 10.0, sr * 0.45);
        double w0 = 2.0 * kPi * freq / sr, cw = std::cos(w0), sw = std::sin(w0);
        double alpha = sw / (2.0 * std::max(q, 0.1)), a0 = 1.0 + alpha;
        b0 = (1.0 + cw) * 0.5 / a0; b1 = -(1.0 + cw) / a0; b2 = (1.0 + cw) * 0.5 / a0;
        a1 = (-2.0 * cw) / a0; a2 = (1.0 - alpha) / a0;
    }

    void setLPF2(double sr, double freq, double q = 0.70710678) noexcept
    {
        freq = std::clamp(freq, 200.0, sr * 0.46);
        double w0 = 2.0 * kPi * freq / sr, cw = std::cos(w0), sw = std::sin(w0);
        double alpha = sw / (2.0 * std::max(q, 0.1)), a0 = 1.0 + alpha;
        b0 = (1.0 - cw) * 0.5 / a0; b1 = (1.0 - cw) / a0; b2 = (1.0 - cw) * 0.5 / a0;
        a1 = (-2.0 * cw) / a0; a2 = (1.0 - alpha) / a0;
    }

    void setPeak(double sr, double freq, double q, double gainDb) noexcept
    {
        if (std::abs(gainDb) < 0.01) { setBypass(); return; }
        freq = std::clamp(freq, 20.0, sr * 0.46);
        q = std::clamp(q, 0.1, 8.0);
        double A = std::pow(10.0, gainDb / 40.0);
        double w0 = 2.0 * kPi * freq / sr, cw = std::cos(w0), sw = std::sin(w0);
        double alpha = sw / (2.0 * q), a0 = 1.0 + alpha / A;
        b0 = (1.0 + alpha * A) / a0; b1 = (-2.0 * cw) / a0; b2 = (1.0 - alpha * A) / a0;
        a1 = (-2.0 * cw) / a0; a2 = (1.0 - alpha / A) / a0;
    }

    void setLowShelf(double sr, double freq, double slope, double gainDb) noexcept
    {
        if (std::abs(gainDb) < 0.01) { setBypass(); return; }
        freq = std::clamp(freq, 20.0, sr * 0.45);
        double A = std::pow(10.0, gainDb / 40.0);
        double w0 = 2.0 * kPi * freq / sr, cw = std::cos(w0), sw = std::sin(w0);
        double s = std::clamp(slope, 0.2, 1.5);
        double alpha = sw * 0.5 * std::sqrt(std::max(0.01, (A + 1.0 / A) * (1.0 / s - 1.0) + 2.0));
        double twoA = 2.0 * std::sqrt(A) * alpha;
        double a0 = (A + 1.0) + (A - 1.0) * cw + twoA;
        b0 = (A * ((A + 1.0) - (A - 1.0) * cw + twoA)) / a0;
        b1 = (2.0 * A * ((A - 1.0) - (A + 1.0) * cw)) / a0;
        b2 = (A * ((A + 1.0) - (A - 1.0) * cw - twoA)) / a0;
        a1 = (-2.0 * ((A - 1.0) + (A + 1.0) * cw)) / a0;
        a2 = ((A + 1.0) + (A - 1.0) * cw - twoA) / a0;
    }

    void setHighShelf(double sr, double freq, double slope, double gainDb) noexcept
    {
        if (std::abs(gainDb) < 0.01) { setBypass(); return; }
        freq = std::clamp(freq, 200.0, sr * 0.45);
        double A = std::pow(10.0, gainDb / 40.0);
        double w0 = 2.0 * kPi * freq / sr, cw = std::cos(w0), sw = std::sin(w0);
        double s = std::clamp(slope, 0.2, 1.5);
        double alpha = sw * 0.5 * std::sqrt(std::max(0.01, (A + 1.0 / A) * (1.0 / s - 1.0) + 2.0));
        double twoA = 2.0 * std::sqrt(A) * alpha;
        double a0 = (A + 1.0) - (A - 1.0) * cw + twoA;
        b0 = (A * ((A + 1.0) + (A - 1.0) * cw + twoA)) / a0;
        b1 = (-2.0 * A * ((A - 1.0) + (A + 1.0) * cw)) / a0;
        b2 = (A * ((A + 1.0) + (A - 1.0) * cw - twoA)) / a0;
        a1 = (2.0 * ((A - 1.0) - (A + 1.0) * cw)) / a0;
        a2 = ((A + 1.0) - (A - 1.0) * cw - twoA) / a0;
    }
};

struct ChannelParams
{
    float lineTrimDb = 0.0f, micGainDb = 0.0f;
    bool pad20Db = false, phaseInvert = false, analogOn = true;
    bool dynBypass = false, dynPostEq = false, splitFilters = false;
    bool eqBypass = false, eqToDynSC = false, extSidechain = false, eqListen = false;
    float compRatio = 1.0f, compThreshDb = 10.0f, compReleaseSec = 0.3f;
    bool compFastAtk = false;
    float expThreshDb = -30.0f, expRangeDb = 0.0f, expReleaseSec = 0.3f;
    bool expFastAtk = false, gateMode = false;
    bool blackEqMode = false;
    float hpfFreqHz = 16.0f, lpfFreqHz = 22000.0f;
    float hfGainDb = 0.0f, hfFreqHz = 8000.0f; bool hfBell = false;
    float hmfGainDb = 0.0f, hmfFreqHz = 2500.0f, hmfQ = 1.5f;
    float lmfGainDb = 0.0f, lmfFreqHz = 800.0f, lmfQ = 1.5f;
    float lfGainDb = 0.0f, lfFreqHz = 120.0f; bool lfBell = false;
    float outputDb = 0.0f, widthNorm = -1.0f;
    bool extraWide = false, widthFilter = false;
};

struct MeterSnapshot
{
    float inputPeakL = 0.0f, inputPeakR = 0.0f;
    float outputPeakL = 0.0f, outputPeakR = 0.0f;
    float compGrDb = 0.0f, gateGrDb = 0.0f;
};

class SSLChannelDSP
{
public:
    void prepare(double newSampleRate) noexcept { sampleRate = std::max(22050.0, newSampleRate); reset(); }

    void reset() noexcept
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            hpf2[ch].reset(); hpf1[ch].reset(); lpf2[ch].reset();
            hf[ch].reset(); hmf[ch].reset(); lmf[ch].reset(); lf[ch].reset();
            dcBlock[ch].reset(); widthLowLpf[ch].reset();
            dcBlock[ch].setOnePoleHPF(sampleRate, 6.0);
            widthLowLpf[ch].setLPF2(sampleRate, 140.0, 0.70710678);
        }
        compGrSmoothDb = 0.0; gateGrSmoothDb = 0.0; gateOpen = false; noiseState = 0x12345678u;
    }

    void updateFilters(const ChannelParams& p) noexcept
    {
        const bool isBlack = p.blackEqMode;
        const double maxGain = isBlack ? 18.0 : 15.0;
        hpfActive = (p.hpfFreqHz > 17.0f);
        lpfActive = (p.lpfFreqHz < 21800.0f);

        for (int ch = 0; ch < 2; ++ch)
        {
            if (hpfActive) { hpf2[ch].setHPF2(sampleRate, p.hpfFreqHz, isBlack ? 0.78 : 0.70710678); if (isBlack) hpf1[ch].setOnePoleHPF(sampleRate, p.hpfFreqHz); else hpf1[ch].setBypass(); }
            else { hpf2[ch].setBypass(); hpf1[ch].setBypass(); }
            if (lpfActive) lpf2[ch].setLPF2(sampleRate, p.lpfFreqHz, 0.70710678); else lpf2[ch].setBypass();

            double hfG = std::clamp(static_cast<double>(p.hfGainDb), -maxGain, maxGain);
            if (p.hfBell) hf[ch].setPeak(sampleRate, p.hfFreqHz, isBlack ? 1.7 : (1.1 + 0.045 * std::abs(hfG)), hfG);
            else hf[ch].setHighShelf(sampleRate, p.hfFreqHz, isBlack ? 0.85 : 0.65, std::clamp(hfG, -15.0, 15.0));

            double hmfG = std::clamp(static_cast<double>(p.hmfGainDb), -maxGain, maxGain);
            double hmfQ = isBlack ? std::clamp(p.hmfQ * 1.15, 0.1, 4.5) : std::clamp(p.hmfQ * (0.65 + 0.032 * std::abs(hmfG)), 0.1, 4.0);
            hmf[ch].setPeak(sampleRate, p.hmfFreqHz, hmfQ, hmfG);

            double lmfG = std::clamp(static_cast<double>(p.lmfGainDb), -maxGain, maxGain);
            double lmfQ = isBlack ? std::clamp(p.lmfQ * 1.15, 0.1, 4.5) : std::clamp(p.lmfQ * (0.65 + 0.032 * std::abs(lmfG)), 0.1, 4.0);
            lmf[ch].setPeak(sampleRate, p.lmfFreqHz, lmfQ, lmfG);

            double lfG = std::clamp(static_cast<double>(p.lfGainDb), -maxGain, maxGain);
            if (p.lfBell) lf[ch].setPeak(sampleRate, p.lfFreqHz, isBlack ? 1.6 : (1.05 + 0.045 * std::abs(lfG)), lfG);
            else lf[ch].setLowShelf(sampleRate, p.lfFreqHz, isBlack ? 0.85 : 0.62, std::clamp(lfG, -15.0, 15.0));
        }
    }

    inline void processStereoSample(float& inL, float& inR, float scL, float scR, const ChannelParams& p, MeterSnapshot& meters) noexcept
    {
        float padGain = p.pad20Db ? 0.1f : 1.0f;
        float lineGain = dbToGain(p.lineTrimDb);
        float micGain = dbToGain(p.micGainDb);
        float phaseSign = p.phaseInvert ? -1.0f : 1.0f;

        double xL = static_cast<double>(inL * padGain * lineGain * phaseSign);
        double xR = static_cast<double>(inR * padGain * lineGain * phaseSign);
        meters.inputPeakL = std::max(meters.inputPeakL, static_cast<float>(std::abs(xL)));
        meters.inputPeakR = std::max(meters.inputPeakR, static_cast<float>(std::abs(xR)));

        if (p.micGainDb > 0.05f || p.analogOn)
        {
            double drive = static_cast<double>(micGain), norm = 1.0 / std::pow(drive, 0.72);
            xL = processPreamp(xL * drive, p.analogOn, p.micGainDb) * norm;
            xR = processPreamp(xR * drive, p.analogOn, p.micGainDb) * norm;
        }
        if (p.analogOn)
        {
            double cL = xR * 1.25e-4, cR = xL * 1.25e-4;
            xL += nextNoise() * 1.5e-5 + cL;
            xR += nextNoise() * 1.5e-5 + cR;
        }

        double extL = static_cast<double>(scL), extR = static_cast<double>(scR);
        if (p.eqToDynSC)
        {
            double keyL = p.extSidechain ? extL : xL, keyR = p.extSidechain ? extR : xR;
            processFilters(keyL, keyR);
            if (!p.eqBypass) process4BandEQ(keyL, keyR, p.analogOn);
            if (p.eqListen) { xL = keyL; xR = keyR; }
            else if (!p.dynBypass) processDynamics(xL, xR, keyL, keyR, p, meters);
        }
        else if (p.dynPostEq)
        {
            processFilters(xL, xR);
            if (!p.eqBypass) process4BandEQ(xL, xR, p.analogOn);
            if (!p.dynBypass) processDynamics(xL, xR, p.extSidechain ? extL : xL, p.extSidechain ? extR : xR, p, meters);
        }
        else
        {
            if (p.splitFilters) processFilters(xL, xR);
            if (!p.dynBypass) processDynamics(xL, xR, p.extSidechain ? extL : xL, p.extSidechain ? extR : xR, p, meters);
            if (!p.splitFilters) processFilters(xL, xR);
            if (!p.eqBypass) process4BandEQ(xL, xR, p.analogOn);
        }

        processStereoWidth(xL, xR, p);
        double outGain = static_cast<double>(dbToGain(p.outputDb));
        xL = dcBlock[0].process(xL * outGain);
        xR = dcBlock[1].process(xR * outGain);
        if (p.analogOn) { xL = 2.0 * std::tanh(xL * 0.5); xR = 2.0 * std::tanh(xR * 0.5); }

        inL = static_cast<float>(xL); inR = static_cast<float>(xR);
        meters.outputPeakL = std::max(meters.outputPeakL, std::abs(inL));
        meters.outputPeakR = std::max(meters.outputPeakR, std::abs(inR));
    }

private:
    double sampleRate = 44100.0;
    bool hpfActive = false, lpfActive = false, gateOpen = false;
    Biquad hpf2[2], hpf1[2], lpf2[2], hf[2], hmf[2], lmf[2], lf[2], dcBlock[2], widthLowLpf[2];
    double compGrSmoothDb = 0.0, gateGrSmoothDb = 0.0;
    uint32_t noiseState = 0x12345678u;

    inline double nextNoise() noexcept { noiseState = noiseState * 1664525u + 1013904223u; return (static_cast<double>(noiseState) / 2147483648.0) - 1.0; }

    inline double processPreamp(double x, bool analogOn, float micDb) noexcept
    {
        double sat = (micDb / 50.0) * 0.85 + (analogOn ? 0.12 : 0.0);
        if (sat < 0.001) return x;
        double asym = x + 0.08 * sat * x * x;
        double shaped = std::tanh(asym * (1.0 + 1.5 * sat)) / (1.0 + 1.5 * sat);
        return x * (1.0 - sat * 0.55) + shaped * (sat * 0.55);
    }

    inline void processFilters(double& l, double& r) noexcept
    {
        if (hpfActive) { l = hpf1[0].process(hpf2[0].process(l)); r = hpf1[1].process(hpf2[1].process(r)); }
        if (lpfActive) { l = lpf2[0].process(l); r = lpf2[1].process(r); }
    }

    inline void process4BandEQ(double& l, double& r, bool analogOn) noexcept
    {
        l = lf[0].process(lmf[0].process(hmf[0].process(hf[0].process(l))));
        r = lf[1].process(lmf[1].process(hmf[1].process(hf[1].process(r))));
        if (analogOn) { l -= 0.006 * l * l * l; r -= 0.006 * r * r * r; }
    }

    inline void processDynamics(double& l, double& r, double keyL, double keyR, const ChannelParams& p, MeterSnapshot& meters) noexcept
    {
        double detDb = 20.0 * std::log10(std::max(std::max(std::abs(keyL), std::abs(keyR)), 1.0e-6));
        double compGrDb = 0.0, autoMakeupDb = 0.0;
        if (p.compRatio > 1.01f)
        {
            double thresh = p.compThreshDb, ratio = (p.compRatio >= 25.0f) ? 100.0 : p.compRatio;
            double overDb = std::max(0.0, detDb - thresh);
            double atkSec = p.compFastAtk ? 0.001 : std::clamp(0.025 - overDb * 0.0008, 0.003, 0.030);
            double relSec = std::clamp(static_cast<double>(p.compReleaseSec), 0.1, 4.0);
            double atkC = std::exp(-1.0 / (atkSec * sampleRate)), relC = std::exp(-1.0 / (relSec * sampleRate));
            constexpr double knee = 4.0;
            double targetGr = 0.0, diff = detDb - thresh;
            if (diff > knee * 0.5) targetGr = diff * (1.0 - 1.0 / ratio);
            else if (diff > -knee * 0.5) { double x = diff + knee * 0.5; targetGr = (1.0 - 1.0 / ratio) * (x * x) / (2.0 * knee); }
            compGrSmoothDb = (targetGr > compGrSmoothDb) ? (atkC * compGrSmoothDb + (1.0 - atkC) * targetGr) : (relC * compGrSmoothDb + (1.0 - relC) * targetGr);
            compGrDb = compGrSmoothDb;
            autoMakeupDb = std::clamp(std::max(0.0, -thresh) * (1.0 - 1.0 / ratio) * 0.45, 0.0, 15.0);
        }
        else { compGrSmoothDb *= 0.995; compGrDb = compGrSmoothDb; }

        double gateGrDb = 0.0;
        if (p.expRangeDb > 0.1f)
        {
            double openT = p.expThreshDb, closeT = openT - (p.gateMode ? 6.0 : 3.0);
            if (!gateOpen && detDb >= openT) gateOpen = true;
            else if (gateOpen && detDb < closeT) gateOpen = false;
            double targetG = 0.0;
            if (!gateOpen) targetG = p.gateMode ? static_cast<double>(p.expRangeDb) : std::min(static_cast<double>(p.expRangeDb), std::max(0.0, openT - detDb) * 1.65);
            double openC = std::exp(-1.0 / ((p.expFastAtk ? 0.001 : 0.025) * sampleRate));
            double closeC = std::exp(-1.0 / (std::clamp(static_cast<double>(p.expReleaseSec), 0.1, 4.0) * sampleRate));
            gateGrSmoothDb = (targetG < gateGrSmoothDb) ? (openC * gateGrSmoothDb + (1.0 - openC) * targetG) : (closeC * gateGrSmoothDb + (1.0 - closeC) * targetG);
            gateGrDb = gateGrSmoothDb;
        }
        else { gateGrSmoothDb *= 0.995; gateGrDb = gateGrSmoothDb; }

        double vca = std::pow(10.0, (autoMakeupDb - compGrDb - gateGrDb) * 0.05);
        l *= vca; r *= vca;
        meters.compGrDb = std::max(meters.compGrDb, static_cast<float>(compGrDb));
        meters.gateGrDb = std::max(meters.gateGrDb, static_cast<float>(gateGrDb));
    }

    inline void processStereoWidth(double& l, double& r, const ChannelParams& p) noexcept
    {
        double sideGain = -static_cast<double>(p.widthNorm) * (p.extraWide ? 1.45 : 1.0);
        double mid = 0.5 * (l + r), side = 0.5 * (l - r);
        if (p.widthFilter) { double lowS = widthLowLpf[0].process(side); side = (side - lowS) * sideGain; }
        else side *= sideGain;
        l = mid + side; r = mid - side;
    }
};
} // namespace SolarSSL
