#include "Dsp.h"
#include <iostream>
#include <stdexcept>
#include <functional>
std::vector<float> render(epic::Settings settings){
 epic::Engine engine;engine.prepare(48000);std::vector<float> out;out.reserve(123000);
 float l[256],r[256];
 for(int block=0;block<240;++block){
  for(int n=0;n<256;++n){int t=block*256+n;l[n]=t<12000?.06f*std::sin(t*.051f):0;r[n]=t<12000?.05f*std::sin(t*.073f):0;}
  engine.process(l,r,256,settings,120);
  for(int n=0;n<256;++n){out.push_back(l[n]);out.push_back(r[n]);}
 }
 return out;
}
int main(){
 epic::Settings base;base.mix=100;for(auto& st:base.strips)st.level=-12;
 const auto reference=render(base);
 auto compare=[&](const char* name,std::function<void(epic::Settings&)> change){
  auto s=base;change(s);auto audio=render(s);double diff=0;
  for(size_t i=0;i<audio.size();++i){if(!std::isfinite(audio[i]))throw std::runtime_error(name);diff+=std::abs(audio[i]-reference[i]);}
  if(diff<.0001)throw std::runtime_error(std::string("control has no audio effect: ")+name);
 };
 for(int i=0;i<4;++i){compare("delay modulation",[i](auto& s){s.modulation[i]=70;});compare("reverb modulation",[i](auto& s){s.reverbMod[i]=70;});}
 for(int i=0;i<2;++i){
  compare("tap 2 time",[i](auto& s){s.time2[i]=53;});compare("tap 2 feedback",[i](auto& s){s.feedback2[i]=60;});
  compare("tap 2 sync",[i](auto& s){s.sync2[i]=true;s.beats2[i]=.25f;});compare("offset",[i](auto& s){s.offset[i]=15;});
 }
 compare("Throw trigger closed",[](auto& s){s.triggerEnabled=true;s.trigger=false;});
 auto gateOpen=base;gateOpen.triggerEnabled=true;gateOpen.trigger=true;
 auto opened=render(gateOpen);if(opened!=reference)throw std::runtime_error("open trigger should pass input");
 compare("Slap VSO",[](auto& s){s.vso=1;});compare("Crowd width",[](auto& s){s.crowdWidth=80;});
 for(int i=0;i<3;++i){compare("reverb low ratio",[i](auto& s){s.toneLow[i]=1.8f;});compare("reverb high ratio",[i](auto& s){s.toneHigh[i]=.4f;});}
 compare("Space nonlinear decay",[](auto& s){s.spaceShape=3.5f;});
 compare("Space low ratio",[](auto& s){s.lowRatio=1.8f;});compare("Space high ratio",[](auto& s){s.highRatio=.4f;});
 compare("delay send",[](auto& s){s.sends[0][0]=90;});compare("direct out",[](auto& s){s.direct[0]=false;});
 compare("filter",[](auto& s){s.strips[0].hp=1500;s.strips[4].lp=1000;});
 compare("mute",[](auto& s){s.strips[0].mute=true;});compare("solo",[](auto& s){s.strips[4].solo=true;});
 std::cout<<"PASS: stereo taps, sync, offset, modulation, trigger, VSO, width, decay ratios, routing, filters and mute/solo affect audio\n";
}
