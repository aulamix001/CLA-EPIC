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
class TestWindow final : public juce::DocumentWindow {
public:
 TestWindow():DocumentWindow("EpicLab UI interaction test",juce::Colours::black,DocumentWindow::closeButton){setUsingNativeTitleBar(true);}
 void closeButtonPressed() override {setVisible(false);}
};
int main(){
 try {
 juce::ScopedJuceInitialiser_GUI gui;
 std::unique_ptr<juce::AudioProcessor> p(createPluginFilter());
 std::unique_ptr<juce::AudioProcessorEditor> editor(p->createEditor());
 check(editor!=nullptr,"editor exists");editor->setSize(1100,705);
 TestWindow window;window.setContentNonOwned(editor.get(),true);window.centreWithSize(1100,705);window.setVisible(true);window.toFront(true);
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
    checkControl(key("lowRatio"),i==7?settings.lowRatio:settings.toneLow[i-4]);checkControl(key("highRatio"),i==7?settings.highRatio:settings.toneHigh[i-4]);
    if(i==7)checkControl(key("shape"),settings.spaceShape);
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
 // Exercise real click-to-edit and the drawn arrow buttons, not just setValue().
 auto select=[&](bool reverb,int i){auto* button=dynamic_cast<juce::Button*>(find(*editor,juce::String(reverb?"reverbTab":"delayTab")+juce::String(i)));check(button!=nullptr,"selector exists");button->onClick();};
 auto parameter=[&](const juce::String& id)->juce::RangedAudioParameter*{for(auto* base:p->getParameters())if(auto* par=dynamic_cast<juce::RangedAudioParameter*>(base))if(par->paramID==id)return par;throw std::runtime_error("missing parameter");};
 auto read=[&](const juce::String& id){auto* par=parameter(id);return par->convertFrom0to1(par->getValue());};
 auto clickEvent=[](juce::Component& c){return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),{float(c.getWidth()/2),float(c.getHeight()/2)},juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),1,0,0,0,0,&c,&c,juce::Time::getCurrentTime(),{float(c.getWidth()/2),float(c.getHeight()/2)},juce::Time::getCurrentTime(),1,false);};
 auto typeValue=[&](const juce::String& id,const juce::String& text){
  auto* slider=dynamic_cast<juce::Slider*>(find(*editor,id));check(slider!=nullptr,"numeric control exists: "+id);
  auto event=clickEvent(*slider);slider->mouseDown(event);slider->mouseUp(event);
  auto* input=dynamic_cast<juce::TextEditor*>(find(*slider,id+"_edit"));check(input&&input->isVisible(),"click opens numeric editor: "+id);
  input->setText(text,false);input->onReturnKey();check(!input->isVisible(),"enter closes numeric editor");
 };
 select(false,0);typeValue("s0_time","321,5 ms");check(std::abs(read("s0_time")-321.5f)<.01f,"typed milliseconds change DSP parameter");
 typeValue("s0_feedback","42.5%");check(std::abs(read("s0_feedback")-42.5f)<.01f,"typed feedback percent");
 typeValue("s0_beats","1/8D");check(std::abs(read("s0_beats")-.75f)<.0001f&&read("s0_sync")==1,"typed dotted note enables tempo sync");
 typeValue("s0_beats","1/0");check(std::abs(read("s0_beats")-.75f)<.0001f,"invalid division rejected");
 select(false,2);typeValue("s2_vso","15 IPS");check(read("s2_vso")==2&&read("s2_sync")==0,"typed VSO tape speed");
 typeValue("s2_time","87 ms");check(std::abs(read("s2_time")-87)<.001f&&read("s2_vso")==0&&read("s2_sync")==0,"manual time exits VSO and sync modes");
 select(true,3);typeValue("s7_shape","2,25");check(std::abs(read("s7_shape")-2.25f)<.0001f,"independent Space decay editing");
 typeValue("s7_decay","2.6sec");check(std::abs(read("s7_decay")-2.6f)<.0001f&&std::abs(read("s7_shape")-2.25f)<.0001f,"reverb time does not change Space shape");
 select(true,0);typeValue("s4_lowRatio","x1,25");check(std::abs(read("s4_lowRatio")-1.25f)<.0001f,"Plate ratio editing");
 typeValue("s4_lowRatio","nan");check(std::abs(read("s4_lowRatio")-1.25f)<.0001f,"nonfinite typed values rejected");
 std::set<juce::String> arrowsTested;
 for(int tab=0;tab<4;++tab){
  select(false,tab);select(true,tab);
  for(auto* base:p->getParameters()){
   auto* par=dynamic_cast<juce::RangedAudioParameter*>(base);if(!par)continue;
   auto* up=dynamic_cast<juce::Button*>(find(*editor,par->paramID+"_up"));auto* down=dynamic_cast<juce::Button*>(find(*editor,par->paramID+"_down"));if(!up||!down)continue;
   auto* slider=dynamic_cast<juce::Slider*>(find(*editor,par->paramID));slider->setValue(par->convertFrom0to1(.5f),juce::sendNotificationSync);
   const auto before=read(par->paramID);up->onClick();const auto after=read(par->paramID);check(after>before,"up arrow controls parameter: "+par->paramID);
   down->onClick();check(read(par->paramID)<after,"down arrow controls parameter: "+par->paramID);arrowsTested.insert(par->paramID);
  }
 }
 check(arrowsTested.size()==39,"all 39 displayed arrow pairs are functional");
 select(false,1);select(true,3);
 for(const auto id:{"s0_hp","s0_lp","s1_mod","s7_mod"}){
  auto* knob=dynamic_cast<juce::Slider*>(find(*editor,id));check(knob!=nullptr,"knob exists");
  knob->setValue(knob->getMinimum(),juce::sendNotificationSync);const auto a=knob->createComponentSnapshot(knob->getLocalBounds());
  knob->setValue(knob->getMaximum(),juce::sendNotificationSync);const auto b=knob->createComponentSnapshot(knob->getLocalBounds());
  const float cx=knob->getWidth()*.5f,cy=knob->getHeight()*.5f;int samples=0;
  for(int y=0;y<a.getHeight();++y)for(int x=0;x<a.getWidth();++x){const float radius=std::hypot(x+.5f-cx,y+.5f-cy);
   if(radius>=(a.getWidth()>60?23.f:20.f)&&radius<=(a.getWidth()>60?29.f:21.f)){check(a.getPixelAt(x,y)==b.getPixelAt(x,y),"knob outer ring stays aligned: "+juce::String(id));++samples;}}
  check(samples>20,"knob alignment samples executed");
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

 // Preserve the frame, geometry and original PNG.
 combo->setSelectedId(1,juce::sendNotificationSync);
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
 auto* selector=find(*editor,"factoryPresets");check(selector&&selector->isVisible()&&selector->getBounds()==juce::Rectangle<int>(1427,328,119,48),"preset selector is visible in right sidebar");
 int presetPixels=0;for(int y=328;y<376;y+=2)for(int x=1427;x<1546;x+=2)if(snapshot.getPixelAt(x,y)!=original.getPixelAt(x,y))++presetPixels;
 check(presetPixels>50,"PRESETS selector is actually drawn rather than transparent");
 check(combo->getText().contains("001")&&combo->getText().contains("Vocal"),"visible preset name and number");
 // Save a separate application screenshot; never modify the immutable source image.
 auto preview=juce::File::getCurrentWorkingDirectory().getChildFile("interface-preview.png");
 preview.deleteFile();if(auto stream=preview.createOutputStream()){juce::PNGImageFormat png;check(png.writeImageToStream(snapshot,*stream),"write preview screenshot");}
 dynamic_cast<juce::Button*>(find(*editor,"delayTab2"))->onClick();dynamic_cast<juce::Button*>(find(*editor,"reverbTab1"))->onClick();
 auto alternate=editor->createComponentSnapshot(editor->getLocalBounds());auto file=juce::File::getCurrentWorkingDirectory().getChildFile("interface-slap-room.png");file.deleteFile();
 if(auto stream=file.createOutputStream()){juce::PNGImageFormat png;check(png.writeImageToStream(alternate,*stream),"write Slap/Room screenshot");}
 editor->setSize(840,540);editor->setSize(1882,1210);window.clearContentComponent();
 std::cout<<"PASS: all "<<visited.size()<<" live parameter controls, automation, 200 preset fader mappings, mouse/keyboard editing, 39 arrow pairs, fixed knob rings and edited state restore\n";
 return 0;
 }catch(const std::exception& error){std::cerr<<"UI ERROR: "<<error.what()<<'\n';return 1;}
}
