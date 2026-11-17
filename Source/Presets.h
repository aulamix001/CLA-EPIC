#pragma once
#include "Dsp.h"
#include <string>
#include <iterator>
namespace epic {
struct Preset { std::string name; Settings settings; };
inline constexpr int factoryPresetCount=200;
// Original EpicLab programs, voiced by source and effect intent. Names/indices
// remain stable; this bank does not reproduce or contain Waves factory presets.
inline const std::array<Preset,factoryPresetCount>& factoryPresets() {
    static const auto bank=[] {
        struct Profile {
            const char* name;
            float hp,lp,decay,predelay,slap,echo,mod,mixBias,room;
        };
        const Profile profiles[]={
            {"Vocal",180,10000,1.6f,22,90,310,6,0,.42f},
            {"Guitar",160,8000,1.3f,12,105,280,4,-2,.36f},
            {"Piano",110,11500,2.4f,18,82,360,3,1,.55f},
            {"Drums",260,8500,.85f,4,65,220,1,-4,.25f},
            {"Snare",320,9500,1.2f,12,78,250,2,-2,.34f},
            {"Synth",120,14000,2.8f,28,115,400,14,6,.65f},
            {"Ambient",220,10000,4.2f,40,125,480,22,10,.8f},
            {"Bass",320,6000,1,5,68,260,1,-5,.28f},
            {"Bus",160,10500,1.4f,16,88,330,2,-4,.4f},
            {"FX",240,13000,3.6f,36,120,440,25,8,.7f},
            {"Vocal Doubles",220,9000,1.4f,12,76,290,10,1,.36f},
            {"Backing Vocals",260,8500,2.3f,30,108,365,12,4,.58f},
            {"Acoustic Guitar",140,12000,1.65f,14,84,275,2,-1,.4f},
            {"Electric Guitar",210,7500,1.8f,18,112,325,8,0,.45f},
            {"Strings",130,11500,3.2f,34,100,410,8,5,.68f},
            {"Brass",240,10000,2.1f,22,86,300,3,1,.48f},
            {"Percussion",280,12500,1.05f,7,64,235,2,-2,.3f},
            {"Electronic Drums",190,15500,1.7f,10,72,375,10,3,.46f},
            {"Orchestral",140,12500,3.8f,42,105,460,7,6,.75f},
            {"Sound Design",200,16000,4.8f,50,128,510,30,12,.85f}
        };
        const char* styles[]={"Close","Slap","Eighth","Quarter","Dotted","Wide","Dark","Bright","Deep","Wash"};
        static_assert(std::size(profiles)*std::size(styles)==factoryPresetCount);
        std::array<Preset,factoryPresetCount> result;
        for(int g=0;g<int(std::size(profiles));++g)for(int v=0;v<int(std::size(styles));++v){
            const auto& p=profiles[g];auto& preset=result[size_t(g*10+v)];auto& s=preset.settings;
            preset.name=std::string(p.name)+" / "+styles[v];s.output=-1.5f;
            for(auto& strip:s.strips){strip.level=-18;strip.hp=p.hp;strip.lp=p.lp;strip.mute=true;strip.solo=false;}
            for(int d=0;d<4;++d){
                s.time[d]=s.time2[d]=p.echo;s.feedback[d]=s.feedback2[d]=25;
                s.beats[d]=s.beats2[d]=.5f;s.sync[d]=s.sync2[d]=false;
                s.offset[d]=0;s.modulation[d]=0;s.direct[d]=true;s.sends[d].fill(0);
                s.decay[d]=std::clamp(p.decay,.1f,10.f);s.predelay[d]=p.predelay;s.reverbMod[d]=p.mod*.3f;
            }
            auto enable=[&](int strip,float level){s.strips[strip].mute=false;s.strips[strip].level=level;};
            auto mix=[&](float value){s.mix=std::clamp(value+p.mixBias,20.f,82.f);};
            switch(v){
                case 0: // Short room ambience; no echo or long tail.
                    mix(36);enable(5,2);s.decay[1]=p.room;s.predelay[1]=std::min(12.f,p.predelay*.25f);s.reverbMod[1]=0;
                    break;
                case 1: // A single, audible manual slap; no tempo-dependent substitution.
                    mix(38);enable(2,0);s.time[2]=p.slap;s.feedback[2]=8;
                    break;
                case 2: // One eighth note in both channels at any host tempo.
                case 3: // One quarter note in both channels at any host tempo.
                case 4: { // Dotted eighth left, dotted quarter right.
                    mix(v==2?40:v==3?43:46);const int d=v==2?0:1;enable(d,0);
                    s.sync[d]=s.sync2[d]=true;s.beats[d]=v==2?.5f:v==3?1.f:.75f;
                    s.beats2[d]=v==4?1.5f:s.beats[d];s.feedback[d]=v==2?28:v==3?38:42;s.feedback2[d]=s.feedback[d];
                    break;
                }
                case 5: // Stereo doubling uses genuinely independent left/right taps.
                    mix(46);enable(0,0);s.time[0]=12+p.slap*.04f;s.time2[0]=27+p.slap*.06f;
                    s.feedback[0]=s.feedback2[0]=0;s.modulation[0]=p.mod*.25f;
                    enable(6,-8);s.decay[2]=std::clamp(p.decay*.6f,.5f,2.2f);s.predelay[2]=18;
                    break;
                case 6: // Matched routing/timing makes Dark/Bright a tonal comparison.
                case 7:
                    mix(48);enable(0,-1);enable(6,-3);s.time[0]=p.echo;s.time2[0]=p.echo*1.25f;
                    s.feedback[0]=s.feedback2[0]=42;s.modulation[0]=p.mod*.35f;
                    s.decay[2]=std::clamp(p.decay*1.25f,1.2f,6.f);s.sends[0][2]=18;
                    for(auto& strip:s.strips)strip.lp=v==6?1800.f:18000.f;
                    break;
                case 8: // Clear leading transient followed by a long hall/plate tail.
                    mix(56);enable(6,2);enable(4,-5);s.decay[2]=std::clamp(p.decay*2.3f,2.8f,8.f);
                    s.decay[0]=std::clamp(p.decay*1.2f,1.4f,4.f);s.predelay[2]=55+p.predelay;s.predelay[0]=35+p.predelay;
                    s.reverbMod[2]=p.mod*.7f;
                    break;
                case 9: // Dense, wet, modulated long space fed by a stereo crowd.
                    mix(70);enable(7,0);enable(6,-6);enable(3,-5);s.time[3]=p.echo*.7f;
                    s.feedback[3]=48;s.crowdWidth=90;s.modulation[3]=18+p.mod*.5f;
                    s.decay[3]=std::clamp(p.decay*3.f,6.f,10.f);s.decay[2]=std::clamp(p.decay*1.8f,3.f,8.f);
                    s.predelay[3]=p.predelay+25;s.reverbMod[3]=20+p.mod;s.sends[3][3]=35;s.spaceShape=.1f;
                    break;
            }
        }
        return result;
    }();
    return bank;
}
}
