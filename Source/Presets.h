#pragma once
#include "Dsp.h"
#include <string>
namespace epic {
struct Preset { std::string name; Settings settings; };
// 10 application profiles x 10 distinct arrangements. Original EpicLab settings.
inline const std::array<Preset,100>& factoryPresets() {
    static const auto bank=[] {
        std::array<Preset,100> result;
        const char* groups[]={"Vocal","Guitar","Piano","Drums","Snare","Synth","Ambient","Bass","Bus","FX"};
        const char* styles[]={"Close","Slap","Eighth","Quarter","Dotted","Wide","Dark","Bright","Deep","Wash"};
        const float mix[]={24,20,22,12,20,28,48,10,12,40};
        const float decay[]={1.8f,1.4f,2.2f,.7f,1.2f,2.8f,6.f,.6f,1.f,4.f};
        const float hp[]={140,180,100,200,300,120,180,350,160,240};
        const float beats[]={.25f,.25f,.5f,1.f,.75f,.5f,1.5f,.5f,2.f,3.f};
        for(int g=0;g<10;++g) for(int v=0;v<10;++v) {
            auto& p=result[g*10+v];p.name=std::string(groups[g])+" / "+styles[v];auto& s=p.settings;
            s.mix=mix[g]+v*.7f; s.output=-1.f;
            for(int i=0;i<8;++i) {
                auto& strip=s.strips[i];
                strip.level=-26.f+float((g+v+i*3)%7);
                strip.hp=hp[g]+i*17.f;
                strip.lp=v==6?3200.f:v==7?16000.f:7500.f+g*300.f;
                strip.mute=true;strip.solo=false;
            }
            const int delay=(g+v)%4, reverb=(g+v/3)%4;
            s.strips[delay].mute=false;s.strips[delay].level=-12.f-v*.6f;
            s.strips[reverb+4].mute=false;s.strips[reverb+4].level=-11.f-v*.4f;
            if(v>=5) {s.strips[(reverb+1)%4+4].mute=false;}
            for(int d=0;d<4;++d) {
                s.time[d]=(v==1?70.f:100.f)+g*11.f+d*37.f+v*9.f;
                s.sync[d]=(v>=2 && v<=4);s.beats[d]=std::min(4.f,beats[v]*(d==1?1.5f:1.f));
                s.feedback[d]=v<2?8.f+g:20.f+v*3.f+d;
                s.time2[d]=s.time[d]*1.5f;s.feedback2[d]=s.feedback[d];s.sync2[d]=s.sync[d];s.beats2[d]=std::min(4.f,s.beats[d]*1.5f);
                s.offset[d]=v>=5?float(v-4):0;s.modulation[d]=float(v*3);s.reverbMod[d]=float(v*2);
                s.direct[d]=true;
                s.sends[d].fill(0);
            }
            s.crowdWidth=20.f+v*7.f;s.lowRatio=1.f;s.highRatio=v==6?.6f:1.f;
            s.sends[delay][reverb]=v<2?10.f:25.f+v*4.f;
            for(int r=0;r<4;++r) {
                s.decay[r]=std::min(10.f,decay[g]*(.65f+v*.12f)+r*.15f);
                s.predelay[r]=g==3?0.f:8.f+v*3.f+r*5.f;
            }
        }
        return result;
    }();
    return bank;
}
}
