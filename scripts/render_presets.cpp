#include "Presets.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <stdexcept>
namespace fs=std::filesystem;
void word(std::ostream& s,uint32_t v,int bytes){for(int i=0;i<bytes;++i)s.put(char((v>>(i*8))&255));}
void wav(const fs::path& path,const std::vector<float>& audio,int sr){
 std::ofstream out(path,std::ios::binary);if(!out)throw std::runtime_error("Cannot create audio example");
 out.write("RIFF",4);word(out,36+uint32_t(audio.size()*2),4);out.write("WAVEfmt ",8);word(out,16,4);word(out,1,2);word(out,2,2);word(out,sr,4);word(out,sr*4,4);word(out,4,2);word(out,16,2);out.write("data",4);word(out,uint32_t(audio.size()*2),4);
 for(float x:audio){if(!std::isfinite(x)||std::abs(x)>=.99f)throw std::runtime_error("Audio demo exceeded headroom");word(out,uint16_t(int16_t(std::lround(x*32767))),2);}
 if(!out)throw std::runtime_error("Cannot write audio example");
}
int main(int argc,char** argv){
 try{
 if(argc!=2)throw std::runtime_error("Usage: preset_demos <output-directory>");const fs::path folder(argv[1]);fs::create_directories(folder);
 constexpr int sr=48000,N=sr*10,B=256;std::vector<float> source(N);uint32_t random=1729;
 const int notes[]={60,64,67,72,67,64,62,67,60,65,69,72,71,67,64,60};
 for(int note=0;note<16;++note){const int start=note*sr/4;const double frequency=440*std::pow(2.,(notes[note]-69)/12.);
  for(int i=0;i<int(sr*.45)&&start+i<N;++i){const double t=double(i)/sr;double tone=0;for(int h=1;h<=6;++h)tone+=std::sin(2*epic::pi*frequency*h*t)/std::pow(double(h),1.6);
   random=random*1664525u+1013904223u;const float noise=float(random>>8)/8388608.f-1;
   source[start+i]+=float(.085*tone*std::exp(-t/.105)*(1-std::exp(-t/.001))+.018*noise*std::exp(-t/.004));
  }
 }
 std::vector<float> dry;dry.reserve(N*2);for(float x:source){dry.push_back(x);dry.push_back(x);}wav(folder/"000 Dry Source.wav",dry,sr);
 for(int preset=0;preset<10;++preset){epic::Engine engine;engine.prepare(sr);const auto& p=epic::factoryPresets()[preset];float l[B]{},r[B]{};
  for(int n=0;n<sr/2;n+=B){std::fill_n(l,B,0);std::fill_n(r,B,0);engine.process(l,r,B,p.settings,120);}
  std::vector<float> audio;audio.reserve(N*2);
  for(int n=0;n<N;n+=B){const int count=std::min(B,N-n);for(int i=0;i<count;++i)l[i]=r[i]=source[n+i];engine.process(l,r,count,p.settings,120);for(int i=0;i<count;++i){audio.push_back(l[i]);audio.push_back(r[i]);}}
  const auto name=std::string(preset<9?"00":"0")+std::to_string(preset+1)+" Vocal - "+p.name.substr(p.name.find(" / ")+3)+".wav";wav(folder/name,audio,sr);
 }
 std::ofstream readme(folder/"README.txt");readme<<"EpicLab 0.6.0 audio examples\n\nOwn synthetic plucked phrase at 120 BPM, stereo PCM16 / 48 kHz.\nThe same mono source drives all ten Vocal variations of the revised 200-program bank.\nEach file has a four-second phrase plus a tail window, without individual normalization.\n000 Dry Source.wav is the unprocessed input. Processed files retain the preset mix and output gain.\nThe ten-second files truncate tails that continue beyond the window.\nThese are EpicLab renders, not Waves recordings or evidence of matching the original.\n";
 std::cout<<"PASS: ten preset audio examples plus dry source at 48 kHz, finite samples and no clipped peaks\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
