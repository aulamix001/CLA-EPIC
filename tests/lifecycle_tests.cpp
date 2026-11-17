#include "Dsp.h"
#include <limits>
#include <iostream>
#include <stdexcept>
void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
int main(){
 epic::Engine engine;epic::Settings s;float l[65],r[65];
 std::fill_n(l,65,.1f);std::fill_n(r,65,-.1f);
 engine.process(l,r,65,s);check(l[0]==.1f&&r[64]==-.1f,"process before setup safely passes audio");
 for(double sr:{0.,-1.,std::numeric_limits<double>::quiet_NaN(),44100.,48000.,96000.,192000.}){
  engine.prepare(sr);check(engine.isPrepared(),"setup readiness");
  for(int count:{0,1,16,32,65}){
   std::fill_n(l,65,.1f);std::fill_n(r,65,-.1f);
   engine.process(l,r,count,s,std::numeric_limits<double>::quiet_NaN());
   for(int i=0;i<count;++i)check(std::isfinite(l[i])&&std::isfinite(r[i]),"finite output across setup/rate/tempo changes");
  }
  engine.release();check(!engine.isPrepared(),"release resets readiness");
  std::fill_n(l,65,.1f);engine.process(l,r,65,s);check(l[64]==.1f,"processing after release is safe");
 }
 engine.process(nullptr,nullptr,0,s);
 std::cout<<"PASS: setup/release, premature processing, invalid rate/tempo and tiny blocks\n";
}
