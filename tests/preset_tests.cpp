#include "Presets.h"
#include <set>
#include <iostream>
#include <stdexcept>
void check(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}
int main(){
 const auto& bank=epic::factoryPresets();check(bank.size()==100,"100 factory presets required");
 std::set<std::string> names, signatures;
 for(const auto& p:bank){
  check(names.insert(p.name).second,"unique names");const auto& s=p.settings;
  std::string signature;
  check(s.mix>=0&&s.mix<=100&&s.input>=-24&&s.input<=24&&s.output>=-24&&s.output<=12,"master ranges");
  for(const auto& st:s.strips){
   check(st.level>=-60&&st.level<=6&&st.hp>=20&&st.hp<=2000&&st.lp>=1000&&st.lp<=20000,"strip ranges");
   check(!st.solo,"presets must clear solo");signature+=std::to_string(st.level)+std::to_string(st.mute);
  }
  for(int i=0;i<4;++i){
   check(s.time[i]>=1&&s.time[i]<=4000&&s.feedback[i]>=0&&s.feedback[i]<=95,"delay ranges");
   check(s.beats[i]>=.125f&&s.beats[i]<=4,"beat ranges");
   check(s.decay[i]>=.1f&&s.decay[i]<=10&&s.predelay[i]>=0&&s.predelay[i]<=500,"reverb ranges");
   for(float send:s.sends[i])check(send>=0&&send<=100,"send ranges");
  }
  check(signatures.insert(signature).second,"unique fader/mute mappings");
  epic::Engine e;e.prepare(48000);double energy=0;float l[256]={},r[256]={};
  for(int block=0;block<200;++block){
   for(int i=0;i<256;++i){l[i]=block<100?.05f*std::sin(float(block*256+i)*.03f):0;r[i]=l[i];}
   e.process(l,r,256,s,120);
   for(int i=0;i<256;++i){check(std::isfinite(l[i])&&std::isfinite(r[i])&&std::abs(l[i])<2&&std::abs(r[i])<2,"stable preset audio");energy+=l[i]*l[i]+r[i]*r[i];}
  }
  check(energy>.001,"preset produces output");
 }
 std::cout<<"PASS: 100 unique presets, parameter ranges, fader mappings and audio rendering\n";
}
