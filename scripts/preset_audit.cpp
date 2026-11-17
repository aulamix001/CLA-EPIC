#include "Presets.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <cstdint>
#include <stdexcept>
int main(int argc,char** argv){
 if(argc!=3&&!(argc==4&&std::string(argv[3])=="--verify")){std::cerr<<"Usage: preset_audit <metrics.csv> <pairs.csv> [--verify]\n";return 2;}
 const int sr=24000,N=sr*4,B=256;std::vector<float> input(N);uint32_t rng=7169;
 for(int n=0;n<sr/25;++n){rng=rng*1664525u+1013904223u;input[n]=.2f*(float(rng>>8)/8388608.f-1)*std::sin(epic::pi*n/float(sr/25));}
 struct Metric {double wetDb,sideDb,centroid,late,brightness,peakL,peakR;};std::vector<Metric> metrics;
 std::vector<std::vector<float>> rendered;std::vector<std::string> names;
 std::ofstream out(argv[1]);out<<"number,name,mix_percent,wet_to_dry_db,side_to_mid_db,wet_centroid_ms,wet_energy_after_500ms_percent,peak,wet_difference_ratio,wet_peak_left_ms,wet_peak_right_ms\n";
 double inEnergy=0;for(float x:input)inEnergy+=2*x*x;
 for(int preset=0;preset<200;++preset){
  const auto& p=epic::factoryPresets()[preset];const auto& s=p.settings;epic::Engine engine;engine.prepare(sr);float l[B]{},r[B]{};
  for(int n=0;n<sr/2;n+=B){std::fill_n(l,B,0);std::fill_n(r,B,0);engine.process(l,r,B,s,120);}
  std::vector<float> signal;signal.reserve(N*2);double wetEnergy=0,mid=0,side=0,late=0,centroid=0,peak=0,derivative=0,previousL=0,previousR=0,peakL=0,peakR=0;int peakIndexL=0,peakIndexR=0;
  const double dryGain=epic::gain(s.input)*epic::gain(s.output)*(1-s.mix*.01f);
  for(int n=0;n<N;n+=B){const int count=std::min(B,N-n);for(int i=0;i<count;++i)l[i]=r[i]=input[n+i];engine.process(l,r,count,s,120);
   for(int i=0;i<count;++i){const double a=l[i]-input[n+i]*dryGain,b=r[i]-input[n+i]*dryGain,e=a*a+b*b;if(!std::isfinite(l[i])||!std::isfinite(r[i]))throw std::runtime_error("Non-finite preset output");derivative+=(a-previousL)*(a-previousL)+(b-previousR)*(b-previousR);previousL=a;previousR=b;if(std::abs(a)>peakL){peakL=std::abs(a);peakIndexL=n+i;}if(std::abs(b)>peakR){peakR=std::abs(b);peakIndexR=n+i;}wetEnergy+=e;centroid+=e*(n+i)*1000./sr;if(n+i>=sr/2)late+=e;mid+=(a+b)*(a+b);side+=(a-b)*(a-b);peak=std::max(peak,double(std::max(std::abs(l[i]),std::abs(r[i]))));signal.push_back(l[i]);signal.push_back(r[i]);}
  }
  auto db=[](double x){return 10*std::log10(std::max(1e-20,x));};
  out<<preset+1<<",\""<<p.name<<"\","<<s.mix<<","<<db(wetEnergy/std::max(1e-20,inEnergy*dryGain*dryGain))<<","<<db(side/std::max(1e-20,mid))<<","<<centroid/std::max(1e-20,wetEnergy)<<","<<100*late/std::max(1e-20,wetEnergy)<<","<<peak<<","<<derivative/std::max(1e-20,wetEnergy)<<","<<peakIndexL*1000./sr<<","<<peakIndexR*1000./sr<<"\n";
  metrics.push_back({db(wetEnergy/std::max(1e-20,inEnergy*dryGain*dryGain)),db(side/std::max(1e-20,mid)),centroid/std::max(1e-20,wetEnergy),100*late/std::max(1e-20,wetEnergy),derivative/std::max(1e-20,wetEnergy),peakIndexL*1000./sr,peakIndexR*1000./sr});
  names.push_back(p.name);rendered.push_back(std::move(signal));
 }
 std::vector<double> correlations;
 std::ofstream pairs(argv[2]);pairs<<"first,second,output_correlation\n";
 for(int group=0;group<20;++group)for(int a=0;a<10;++a)for(int b=a+1;b<10;++b){const auto& x=rendered[group*10+a];const auto& y=rendered[group*10+b];double xx=0,yy=0,xy=0;for(size_t i=0;i<x.size();++i){xx+=double(x[i])*x[i];yy+=double(y[i])*y[i];xy+=double(x[i])*y[i];}correlations.push_back(xy/std::sqrt(xx*yy));pairs<<"\""<<names[group*10+a]<<"\",\""<<names[group*10+b]<<"\","<<xy/std::sqrt(xx*yy)<<"\n";}
 if(argc==4){
  auto require=[](bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);};
  for(int g=0;g<20;++g){const auto start=size_t(g*10);const auto& close=metrics[start];
   for(int v=0;v<10;++v)require(metrics[start+v].wetDb>-18,"Effect is buried below dry: "+names[start+v]);
   require(close.centroid<140,"Close ambience is too long: "+names[start]);
   require(metrics[start+1].peakL>=60&&metrics[start+1].peakL<180,"Slap is not a short echo: "+names[start+1]);
   for(int v=2;v<=4;++v){const double expected=v==2?250:v==3?500:375,right=v==4?750:expected;const auto& m=metrics[start+v];
    require(m.peakL>=expected-1&&m.peakL<expected+41&&m.peakR>=right-1&&m.peakR<right+41,"Musical delay timing does not match name: "+names[start+v]);}
   require(metrics[start+5].sideDb>-10,"Wide does not spread a mono source: "+names[start+5]);
   require(metrics[start+7].brightness>metrics[start+6].brightness*1.25,"Dark/Bright lack spectral contrast: "+names[start+6]);
   require(metrics[start+8].centroid>close.centroid*2,"Deep is not deeper than Close: "+names[start+8]);
   require(metrics[start+9].late>close.late+5,"Wash lacks a sustained tail: "+names[start+9]);
  }
  std::sort(correlations.begin(),correlations.end());require(correlations[correlations.size()/2]<.95,"Preset bank is dominated by common dry signal");
  std::cout<<"PASS: all 200 wet responses, 60 musical stereo delay timings, 20 Wide responses, 20 Dark/Bright contrasts and 40 long-tail comparisons\n";
 }
 std::cout<<"Measured 200 presets at 24 kHz, 120 BPM, four-second stereo render, identical 40 ms broadband burst.\n";
}
