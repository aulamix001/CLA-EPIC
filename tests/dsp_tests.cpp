#include "Dsp.h"
#include <iostream>
#include <stdexcept>
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){
 for(double sr:{44100.,48000.,96000.}) {
  epic::Engine e; e.prepare(sr); epic::Settings s;
  constexpr int n=256; float l[n]={},r[n]={};
  for(int b=0;b<20;++b){e.process(l,r,n,s);for(int i=0;i<n;++i)require(l[i]==0&&r[i]==0,"silence must stay silent");}
  s.mix=0;
  for(int b=0;b<120;++b){std::fill_n(l,n,.1f);std::fill_n(r,n,-.1f);e.process(l,r,n,s);}
  require(std::abs(l[255]-.1f)<.00001f && std::abs(r[255]+.1f)<.00001f,"dry stereo passthrough");
  e.prepare(sr); s.mix=100; s.strips[0].level=0; s.time[0]=20;s.feedback[0]=0;
  for(int i=1;i<8;++i)s.strips[i].mute=true;
  for(int b=0;b<40;++b){std::fill_n(l,n,0);std::fill_n(r,n,0);e.process(l,r,n,s);}
  double energy=0; l[0]=1;
  for(int b=0;b<40;++b){if(b>0)std::fill_n(l,n,0);std::fill_n(r,n,0);e.process(l,r,n,s);for(int i=0;i<n;++i){energy+=l[i]*l[i];require(std::isfinite(l[i])&&r[i]==0,"delay stereo isolation");}}
  require(energy>.01,"delay impulse response");
  e.prepare(sr);s=epic::Settings{};s.mix=100;for(auto& st:s.strips)st.mute=true;s.strips[7].mute=false;s.strips[7].level=0;
  for(int b=0;b<40;++b){std::fill_n(l,n,0);std::fill_n(r,n,0);e.process(l,r,n,s);}
  energy=0;l[0]=1;
  for(int b=0;b<200;++b){if(b>0)std::fill_n(l,n,0);std::fill_n(r,n,0);e.process(l,r,n,s);for(float v:l){require(std::isfinite(v)&&std::abs(v)<2,"stable reverb");energy+=v*v;}}
  require(energy>.001,"reverb impulse response");
 }
 std::cout<<"PASS: silence, dry stereo, delay impulse/isolation and reverb impulse/stability at 44.1/48/96 kHz\n";
}
