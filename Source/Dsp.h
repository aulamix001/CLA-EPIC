#pragma once
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>

namespace epic {
constexpr float pi = 3.14159265358979323846f;
inline float gain(float db) { return db <= -60.f ? 0.f : std::pow(10.f, db / 20.f); }
struct Delay {
    std::vector<float> data;
    size_t pos = 0;
    void prepare(double sr, float seconds = 4.1f) { data.assign(static_cast<size_t>(sr * seconds) + 4, 0.f); pos = 0; }
    float read(float samples) const {
        const auto d = std::clamp(samples, 1.f, float(data.size() - 2));
        const auto whole = static_cast<size_t>(d);
        const auto index = (pos + data.size() - whole) % data.size();
        const float a = data[index], b = data[(index + data.size() - 1) % data.size()];
        return a + (b - a) * (d - float(whole));
    }
    void push(float x) { data[pos] = x; pos = (pos + 1) % data.size(); }
};
struct Filter {
    float low = 0, highLow = 0;
    float process(float x, float hpCoeff, float lpCoeff) {
        highLow += hpCoeff * (x - highLow);
        low += lpCoeff * (x - highLow - low);
        return low;
    }
};
struct Strip {
    float level = -18, hp = 20, lp = 20000;
    bool mute = false, solo = false;
};
struct Settings {
    float input = 0, output = 0, mix = 30;
    std::array<Strip, 8> strips {};
    std::array<float, 4> time {120,250,100,35}, feedback {25,35,10,15};
    std::array<bool, 4> sync {}, sync2 {};
    std::array<float,4> time2 {180,375,100,35}, feedback2 {25,35,10,15}, beats2 {.75f,.75f,.25f,.125f};
    std::array<float,4> offset {}, modulation {};
    std::array<float,4> reverbMod {};
    bool triggerEnabled=false, trigger=false;
    float crowdWidth=35, vso=0, lowRatio=1, highRatio=1;
    std::array<float, 4> beats {.5f,.5f,.25f,.125f};
    std::array<float, 4> decay {1.5f, .8f, 2.5f, 4.f}, predelay {0,0,10,20};
    std::array<std::array<float, 4>, 4> sends {};
    std::array<bool, 4> direct {true,true,true,true};
};
// Independent delay/comb-network approximation; does not implement Waves' DSP.
class Engine {
    double sampleRate = 48000;
    bool prepared = false;
    std::array<std::array<Delay,2>,4> delays, pre;
    std::array<std::array<std::array<Delay,4>,2>,4> combs;
    std::array<std::array<Filter,2>,8> filters {};
    std::array<std::array<float,2>,4> currentTime {};
    std::array<float,8> phase {};
    std::array<float,8> levels {};
    float inputGain = 1, outputGain = 1, wet = .3f;
    static constexpr float combMs[4][4] = {{29.7f,37.1f,41.1f,43.7f},{11.7f,17.3f,19.9f,23.3f},{43.7f,53.1f,61.7f,71.3f},{67.3f,79.7f,89.3f,101.1f}};
public:
    bool isPrepared() const noexcept {return prepared;}
    void release() noexcept {prepared=false;}
    void prepare(double sr) {
        prepared=false;
        sr=std::isfinite(sr)&&sr>=8000&&sr<=768000?sr:48000;
        sampleRate = sr; filters = {}; levels.fill(0); for(auto& t:currentTime)t.fill(1); phase.fill(0);
        inputGain = outputGain = 1; wet = .3f;
        for(auto& bank:delays) for(auto& d:bank) d.prepare(sr);
        for(auto& bank:pre) for(auto& d:bank) d.prepare(sr, .51f);
        for(auto& bank:combs) for(auto& ch:bank) for(auto& d:ch) d.prepare(sr, .15f);
        prepared=true;
    }
    void process(float* left, float* right, int count, const Settings& s, double bpm = 120) {
        if(!prepared || count<=0 || left==nullptr || right==nullptr)return;
        if(!std::isfinite(bpm))bpm=120;
        const float targetInput=gain(s.input),targetOutput=gain(s.output);
        bool solo = false; for(const auto& strip:s.strips) solo |= strip.solo;
        std::array<float,8> hp {}, lp {}, target {};
        std::array<std::array<float,2>,4> times {};
        std::array<std::array<std::array<float,4>,2>,4> taps {}, feedbacks {};
        for(int i=0;i<8;++i) {
            hp[i]=1-std::exp(-2*pi*std::clamp(s.strips[i].hp, 10.f, float(sampleRate*.45))/float(sampleRate));
            lp[i]=1-std::exp(-2*pi*std::clamp(s.strips[i].lp, 10.f, float(sampleRate*.45))/float(sampleRate));
            target[i]=(!s.strips[i].mute && (!solo || s.strips[i].solo)) ? gain(s.strips[i].level):0;
        }
        for(int i=0;i<4;++i) {
            for(int ch=0;ch<2;++ch) {
                const bool second=ch==1 && i<2;
                const bool sync=second?s.sync2[i]:s.sync[i];
                float ms=sync?float(60000/std::clamp(bpm,20.,400.))*(second?s.beats2[i]:s.beats[i]):(second?s.time2[i]:s.time[i]);
                if(i==2 && s.vso>0 && !sync) ms=s.vso<1.5f?160.f:s.vso<2.5f?80.f:40.f;
                if(i==3) ms+=s.crowdWidth*(ch==0?1.1f:1.37f);
                times[i][ch]=float(sampleRate*.001)*std::clamp(ms*(1+(ch==0?1:-1)*s.offset[i]*.01f),1.f,4000.f);
            }
            for(int ch=0;ch<2;++ch) for(int k=0;k<4;++k) {
                taps[i][ch][k]=float(sampleRate*.001)*(combMs[i][k]+ch*.71f);
                feedbacks[i][ch][k]=std::pow(.001f, (taps[i][ch][k]/float(sampleRate))/std::max(.1f,s.decay[i]));
                if(i==3)feedbacks[i][ch][k]=std::pow(feedbacks[i][ch][k],1.f/std::max(.25f,k<2?s.lowRatio:s.highRatio));
            }
        }
        const float smoothing=1-std::exp(-1.f/float(sampleRate*.02));
        for(int n=0;n<count;++n) {
            inputGain += smoothing*(targetInput-inputGain);
            outputGain += smoothing*(targetOutput-outputGain);
            wet += smoothing*(s.mix*.01f-wet);
            for(int i=0;i<8;++i) levels[i]+=smoothing*(target[i]-levels[i]);
            for(int i=0;i<4;++i) {
                for(int ch=0;ch<2;++ch) currentTime[i][ch]+=smoothing*(times[i][ch]-currentTime[i][ch]);
                phase[i]+=2*pi*(.15f+s.modulation[i]*.018f)/float(sampleRate);
                phase[i+4]+=2*pi*(.11f+s.reverbMod[i]*.013f)/float(sampleRate);
                if(phase[i]>2*pi)phase[i]-=2*pi;
                if(phase[i+4]>2*pi)phase[i+4]-=2*pi;
            }
            float in[2]={left[n]*inputGain,right[n]*inputGain}, result[2]={};
            for(int ch=0;ch<2;++ch) {
                float dly[4]={};
                for(int i=0;i<4;++i) {
                    const float motion=s.modulation[i]==0?0:std::sin(phase[i]+ch*pi*.5f)*s.modulation[i]*.00002f*float(sampleRate);
                    float tap=delays[i][ch].read(currentTime[i][ch]+motion);
                    if(i==3) tap=.6f*tap+.27f*delays[i][ch].read(currentTime[i][ch]*1.71f)+.13f*delays[i][ch].read(currentTime[i][ch]*2.53f);
                    const bool open=i!=1 || !s.triggerEnabled || s.trigger;
                    float feed=(open?in[ch]:0)+tap*std::clamp((i<2 && ch==1?s.feedback2[i]:s.feedback[i])*.01f,0.f,.95f);
                    if(i==0) feed=std::tanh(feed); // Tape-like saturation
                    delays[i][ch].push(feed);
                    dly[i]=filters[i][ch].process(tap,hp[i],lp[i])*levels[i];
                    if(s.direct[i]) result[ch]+=dly[i];
                }
                for(int r=0;r<4;++r) {
                    float source=in[ch];
                    for(int d=0;d<4;++d) source+=dly[d]*s.sends[d][r]*.01f;
                    pre[r][ch].push(source);
                    float pred=s.predelay[r]<=0?source:pre[r][ch].read(float(sampleRate*.001)*s.predelay[r]+1);
                    float sum=0;
                    for(int k=0;k<4;++k) {
                        auto& comb=combs[r][ch][k];
                        const float motion=s.reverbMod[r]==0?0:std::sin(phase[r+4]+ch+k)*s.reverbMod[r]*.000005f*float(sampleRate);
                        const float v=comb.read(taps[r][ch][k]+motion);
                        comb.push(pred+v*feedbacks[r][ch][k]); sum+=v*.25f;
                    }
                    result[ch]+=filters[r+4][ch].process(sum,hp[r+4],lp[r+4])*levels[r+4];
                }
            }
            left[n]=outputGain*(in[0]*(1-wet)+result[0]*wet);
            right[n]=outputGain*(in[1]*(1-wet)+result[1]*wet);
        }
    }
};
}
