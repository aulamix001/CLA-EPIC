#include "Presets.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <cstdint>
int main(int argc,char** argv){
 if(argc!=3){std::cerr<<"Usage: preset_audit <metrics.csv> <pairs.csv>\n";return 2;}
 const int sr=24000,N=sr*4,B=256;std::vector<float> input(N);uint32_t rng=7169;
 for(int n=0;n<sr/25;++n){rng=rng*1664525u+1013904223u;input[n]=.2f*(float(rng>>8)/8388608.f-1)*std::sin(epic::pi*n/float(sr/25));}
 std::vector<std::vector<float>> rendered;std::vector<std::string> names;
 std::ofstream out(argv[1]);out<<"number,name,mix_percent,wet_to_dry_db,side_to_mid_db,wet_centroid_ms,wet_energy_after_500ms_percent,peak\n";
 double inEnergy=0;for(float x:input)inEnergy+=2*x*x;
 for(int preset=0;preset<200;++preset){
  const auto& p=epic::factoryPresets()[preset];const auto& s=p.settings;epic::Engine engine;engine.prepare(sr);float l[B]{},r[B]{};
  for(int n=0;n<sr/2;n+=B){std::fill_n(l,B,0);std::fill_n(r,B,0);engine.process(l,r,B,s,120);}
  std::vector<float> signal;signal.reserve(N*2);double wetEnergy=0,mid=0,side=0,late=0,centroid=0,peak=0;
  const double dryGain=epic::gain(s.input)*epic::gain(s.output)*(1-s.mix*.01f);
  for(int n=0;n<N;n+=B){const int count=std::min(B,N-n);for(int i=0;i<count;++i)l[i]=r[i]=input[n+i];engine.process(l,r,count,s,120);
   for(int i=0;i<count;++i){const double a=l[i]-input[n+i]*dryGain,b=r[i]-input[n+i]*dryGain,e=a*a+b*b;wetEnergy+=e;centroid+=e*(n+i)*1000./sr;if(n+i>=sr/2)late+=e;mid+=(a+b)*(a+b);side+=(a-b)*(a-b);peak=std::max(peak,double(std::max(std::abs(l[i]),std::abs(r[i]))));signal.push_back(l[i]);signal.push_back(r[i]);}
  }
  auto db=[](double x){return 10*std::log10(std::max(1e-20,x));};
  out<<preset+1<<",\""<<p.name<<"\","<<s.mix<<","<<db(wetEnergy/std::max(1e-20,inEnergy*dryGain*dryGain))<<","<<db(side/std::max(1e-20,mid))<<","<<centroid/std::max(1e-20,wetEnergy)<<","<<100*late/std::max(1e-20,wetEnergy)<<","<<peak<<"\n";
  names.push_back(p.name);rendered.push_back(std::move(signal));
 }
 std::ofstream pairs(argv[2]);pairs<<"first,second,output_correlation\n";
 for(int group=0;group<20;++group)for(int a=0;a<10;++a)for(int b=a+1;b<10;++b){const auto& x=rendered[group*10+a];const auto& y=rendered[group*10+b];double xx=0,yy=0,xy=0;for(size_t i=0;i<x.size();++i){xx+=double(x[i])*x[i];yy+=double(y[i])*y[i];xy+=double(x[i])*y[i];}pairs<<"\""<<names[group*10+a]<<"\",\""<<names[group*10+b]<<"\","<<xy/std::sqrt(xx*yy)<<"\n";}
 std::cout<<"Measured 200 presets at 24 kHz, 120 BPM, four-second stereo render, identical 40 ms broadband burst.\n";
}
