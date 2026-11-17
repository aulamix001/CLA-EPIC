#include <juce_audio_utils/juce_audio_utils.h>
#include "Presets.h"
#include <BinaryData.h>
#include <iostream>
#include <set>
#include <stdexcept>
extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
void check(bool ok,const juce::String& text){if(!ok)throw std::runtime_error(text.toStdString());}
juce::Component* find(juce::Component& c,const juce::String& id){
 if(c.getComponentID()==id)return &c;
 for(auto* child:c.getChildren())if(auto* result=find(*child,id))return result;
 return nullptr;
}
int main(){
 try {
 juce::ScopedJuceInitialiser_GUI gui;
 std::unique_ptr<juce::AudioProcessor> p(createPluginFilter());
 std::unique_ptr<juce::AudioProcessorEditor> editor(p->createEditor());
 check(editor!=nullptr,"editor exists");editor->setSize(1100,748);
 auto* combo=dynamic_cast<juce::ComboBox*>(find(*editor,"factoryPresets"));check(combo&&combo->getNumItems()==200,"200 presets in live selector");
 std::set<juce::String> visited;
 for(int tab=0;tab<4;++tab){
  for(auto prefix:{"delayTab","reverbTab"}){
   auto* button=dynamic_cast<juce::TextButton*>(find(*editor,juce::String(prefix)+juce::String(tab)));check(button!=nullptr,"effect selector exists");button->onClick();
  }
  for(auto* base:p->getParameters()){
   auto* param=dynamic_cast<juce::RangedAudioParameter*>(base);check(param!=nullptr,"ranged parameter");
   auto* c=find(*editor,param->paramID);if(!c)continue;visited.insert(param->paramID);
   check(c->getWidth()>0&&c->getHeight()>0,"control has live bounds");
   if(auto* slider=dynamic_cast<juce::Slider*>(c)){
    const float real=param->convertFrom0to1(.71f);slider->setValue(real,juce::sendNotificationSync);
    check(std::abs(param->convertFrom0to1(param->getValue())-float(slider->getValue()))<.01f,"slider edits audio parameter: "+param->paramID);
    param->setValueNotifyingHost(.23f);
    check(std::abs(param->convertFrom0to1(param->getValue())-float(slider->getValue()))<.01f,"automation updates knob/fader: "+param->paramID);
   }else if(auto* button=dynamic_cast<juce::Button*>(c)){
    param->setValueNotifyingHost(1.f);check(button->getToggleState(),"button follows parameter on");
    param->setValueNotifyingHost(0.f);check(!button->getToggleState(),"button follows parameter off");
   }else check(false,"parameter has interactive component");
  }
 }
 check(visited.size()==size_t(p->getParameters().size()),"every parameter has a live control");
 combo->setSelectedId(0,juce::dontSendNotification);
 check(p->getNumPrograms()==200,"host exposes 200 factory programs");
 for(int n=0;n<p->getNumPrograms();++n){
  combo->setSelectedId(n+1,juce::sendNotificationSync);check(p->getCurrentProgram()==n,"preset selector updates processor");
  const auto& settings=epic::factoryPresets()[size_t(n)].settings;
  auto checkControl=[&](const juce::String& key,float expected){
   auto* c=find(*editor,key);check(c!=nullptr,"preset control exists: "+key);
   if(auto* slider=dynamic_cast<juce::Slider*>(c))check(std::abs(slider->getValue()-expected)<.02,"preset knob/fader mapping: "+key);
   else if(auto* button=dynamic_cast<juce::Button*>(c))check(button->getToggleState()==(expected>.5f),"preset button mapping: "+key);
   else check(false,"unsupported preset control");
  };
  checkControl("input",settings.input);checkControl("output",settings.output);checkControl("mix",settings.mix);
  for(int i=0;i<8;++i){
   auto key=[&](const char* f){return "s"+juce::String(i)+"_"+f;};
   auto* tab=dynamic_cast<juce::Button*>(find(*editor,juce::String(i<4?"delayTab":"reverbTab")+juce::String(i%4)));tab->onClick();
   const auto& strip=settings.strips[i];
   checkControl(key("level"),strip.level);checkControl(key("hp"),strip.hp);checkControl(key("lp"),strip.lp);
   checkControl(key("mute"),strip.mute);checkControl(key("solo"),strip.solo);
   if(i<4){
    checkControl(key("time"),settings.time[i]);checkControl(key("feedback"),settings.feedback[i]);checkControl(key("beats"),settings.beats[i]);checkControl(key("sync"),settings.sync[i]);
    checkControl(key("mod"),settings.modulation[i]);checkControl(key("direct"),settings.direct[i]);
    for(int r=0;r<4;++r)checkControl(key(("send"+juce::String(r)).toRawUTF8()),settings.sends[i][r]);
    if(i<2){checkControl(key("time2"),settings.time2[i]);checkControl(key("feedback2"),settings.feedback2[i]);checkControl(key("beats2"),settings.beats2[i]);checkControl(key("sync2"),settings.sync2[i]);checkControl(key("offset"),settings.offset[i]);}
    if(i==1){checkControl(key("triggerEnabled"),settings.triggerEnabled);checkControl(key("trigger"),settings.trigger);}
    if(i==2)checkControl(key("vso"),settings.vso);if(i==3)checkControl(key("width"),settings.crowdWidth);
   }else{
    checkControl(key("decay"),settings.decay[i-4]);checkControl(key("predelay"),settings.predelay[i-4]);checkControl(key("mod"),settings.reverbMod[i-4]);
    if(i==7){checkControl(key("lowRatio"),settings.lowRatio);checkControl(key("highRatio"),settings.highRatio);}
   }
  }

  for(int strip=0;strip<8;++strip){
   auto key="s"+juce::String(strip)+"_level";
   auto* slider=dynamic_cast<juce::Slider*>(find(*editor,key));
   check(slider&&std::abs(slider->getValue()-settings.strips[strip].level)<.01,"preset maps all 8 faders");
   auto* mute=dynamic_cast<juce::Button*>(find(*editor,"s"+juce::String(strip)+"_mute"));
   check(mute&&mute->getToggleState()==settings.strips[strip].mute,"preset maps mute LEDs");
  }
 }
 // Edited values survive state restore rather than reloading the factory defaults.
 std::vector<float> saved;
 int editIndex=0;
 for(auto* base:p->getParameters()){
  auto* param=dynamic_cast<juce::RangedAudioParameter*>(base);
  // Boolean switches have only 0/1 states. APVTS correctly canonicalises
  // fractional boolean host values; compare valid parameter states here.
  const float edited=param->isBoolean()?float((editIndex++)%2):param->convertTo0to1(param->convertFrom0to1(.37f));
  param->setValueNotifyingHost(edited);saved.push_back(param->getValue());
 }
 juce::MemoryBlock block;p->getStateInformation(block);p->setCurrentProgram(0);p->setStateInformation(block.getData(),int(block.getSize()));
 for(int i=0;i<p->getParameters().size();++i)check(std::abs(p->getParameters()[i]->getValue()-saved[size_t(i)])<.0001f,"session restores edited parameters: "+p->getParameters()[i]->getName(100)+" expected="+juce::String(saved[size_t(i)],8)+" actual="+juce::String(p->getParameters()[i]->getValue(),8));
 check(p->getCurrentProgram()==199,"session restores extended preset index");
 p->setCurrentProgram(-1);p->setCurrentProgram(200);
 check(p->getCurrentProgram()==199,"invalid indices preserve current program");
 check(p->getProgramName(-1).isEmpty()&&p->getProgramName(200).isEmpty(),"invalid names are empty");
 check(p->getProgramName(99)=="FX / Wash","legacy slot 99 retains its name");

 // Preserve the original geometry: no toolbar, replacement panels or generic skin.
 p->setCurrentProgram(0);
 dynamic_cast<juce::Button*>(find(*editor,"delayTab1"))->onClick();
 dynamic_cast<juce::Button*>(find(*editor,"reverbTab3"))->onClick();
 editor->setSize(1567,1004);
 auto original=juce::ImageCache::getFromMemory(BinaryData::imagem_png,BinaryData::imagem_pngSize);
 auto snapshot=editor->createComponentSnapshot(editor->getLocalBounds());
 check(snapshot.getWidth()==original.getWidth()&&snapshot.getHeight()==original.getHeight(),"skin native dimensions failed");
 const juce::Rectangle<int> immutableZones[]={
  {0,0,1567,20},{0,984,1567,20},{0,0,16,1004},{1549,0,18,1004},
  {150,210,24,700},{778,20,18,955},{1406,210,16,700},
  {1428,246,115,77},{180,382,1218,10},{196,507,1196,20}
 };
 for(const auto& zone:immutableZones)for(int y=zone.getY();y<zone.getBottom();y+=3)for(int x=zone.getX();x<zone.getRight();x+=3)
  check(snapshot.getPixelAt(x,y)==original.getPixelAt(x,y),"original skin pixel mapping mismatch at "+juce::String(x)+","+juce::String(y));
 auto* firstFader=find(*editor,"s0_level");check(firstFader&&firstFader->getBounds()==juce::Rectangle<int>(224,578,44,342),"fader stays at original position");
 auto* logo=find(*editor,"factoryPresets");check(logo&&logo->getBounds()==juce::Rectangle<int>(1427,245,119,79),"presets use original logo, no extra toolbar");
 // Save a separate application screenshot; never modify the immutable source image.
 auto preview=juce::File::getCurrentWorkingDirectory().getChildFile("interface-preview.png");
 preview.deleteFile();if(auto stream=preview.createOutputStream()){juce::PNGImageFormat png;check(png.writeImageToStream(snapshot,*stream),"write preview screenshot");}
 editor->setSize(840,540);editor->setSize(1882,1210);
 std::cout<<"PASS: all "<<visited.size()<<" live parameter controls, automation, 200 preset fader mappings and edited state restore\n";
 return 0;
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
