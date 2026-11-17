#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <thread>
#include <iostream>
#include <stdexcept>
void check(bool ok,const juce::String& message){if(!ok)throw std::runtime_error(message.toStdString());}
void stage(const char* text){std::cout<<"HOST "<<text<<std::endl;}
void pump(int ms){juce::MessageManager::getInstance()->runDispatchLoopUntil(ms);}
class Window final : public juce::DocumentWindow {
public:
 Window():DocumentWindow("EpicLab VST3 host validation",juce::Colours::black,DocumentWindow::closeButton){setUsingNativeTitleBar(true);}
 void closeButtonPressed() override {setVisible(false);}
};
int main(int argc,char** argv){
 try{
  juce::ScopedJuceInitialiser_GUI gui;check(argc==2,"VST3 path required");
  auto binary=juce::File(juce::String::fromUTF8(argv[1]));check(binary.existsAsFile(),"compiled DLL exists");
  const auto bundle=binary.getParentDirectory().getParentDirectory().getParentDirectory();
  juce::AudioPluginFormatManager formats;formats.addFormat(new juce::VST3PluginFormat());
  juce::OwnedArray<juce::PluginDescription> types;
  stage("scan real VST3 bundle");formats.getFormat(0)->findAllTypesForFile(types,bundle.getFullPathName());check(types.size()>0,"scan must discover a plugin");
  stage("create real VST3 instance");juce::String error;
  auto plugin=formats.createPluginInstance(*types[0],48000,64,error);check(plugin!=nullptr,"VST3 instantiation: "+error);
  check(plugin->getNumPrograms()==200,"VST3 host exposes 200 programs");
  stage("mono/stereo layouts and repeated setup/release");
  for(int mode=0;mode<3;++mode){
   auto layout=plugin->getBusesLayout();layout.inputBuses.getReference(0)=mode<2?juce::AudioChannelSet::mono():juce::AudioChannelSet::stereo();
   layout.outputBuses.getReference(0)=mode==0?juce::AudioChannelSet::mono():juce::AudioChannelSet::stereo();
   check(plugin->setBusesLayout(layout),"host accepts mono, mono-to-stereo and stereo layouts");
   for(double sr:{44100.,48000.,96000.})for(int block:{16,64,256}){
    plugin->setRateAndBufferSizeDetails(sr,block);plugin->prepareToPlay(sr,block);
    juce::AudioBuffer<float> audio(plugin->getTotalNumOutputChannels(),block);juce::MidiBuffer midi;
    for(int n=0;n<8;++n){audio.clear();audio.setSample(0,0,.05f);plugin->processBlock(audio,midi);
     for(int ch=0;ch<audio.getNumChannels();++ch)for(int i=0;i<block;++i)check(std::isfinite(audio.getSample(ch,i)),"finite host audio");}
    plugin->releaseResources();
   }
  }
  stage("open native editor, restore state and switch every preset");
  plugin->setRateAndBufferSizeDetails(48000,64);plugin->prepareToPlay(48000,64);
  for(int cycle=0;cycle<3;++cycle){
   std::unique_ptr<juce::AudioProcessorEditor> editor(plugin->createEditorIfNeeded());check(editor!=nullptr,"VST3 editor opens");
   Window window;window.setContentNonOwned(editor.get(),true);window.centreWithSize(editor->getWidth(),editor->getHeight());window.setVisible(true);pump(100);
   if(cycle==0){
    juce::AudioBuffer<float> audio(2,64);juce::MidiBuffer midi;
    for(int preset=0;preset<200;++preset){
     plugin->setCurrentProgram(preset);check(plugin->getCurrentProgram()==preset,"VST3 program change");
     audio.clear();plugin->processBlock(audio,midi);
     if(preset%25==0){std::cout<<"HOST preset "<<preset<<std::endl;pump(2);}
    }
    juce::MemoryBlock state;plugin->getStateInformation(state);check(state.getSize()>0,"host state is nonempty");
    plugin->setCurrentProgram(0);plugin->setStateInformation(state.getData(),int(state.getSize()));pump(30);
    check(plugin->getCurrentProgram()==199,"VST3 state restores extended program");
   }
   window.setVisible(false);window.clearContentComponent();editor.reset();pump(30);
  }
  stage("concurrent audio processing and UI automation");
  std::unique_ptr<juce::AudioProcessorEditor> liveEditor(plugin->createEditorIfNeeded());check(liveEditor!=nullptr,"editor reopens for concurrent automation");
  Window liveWindow;liveWindow.setContentNonOwned(liveEditor.get(),true);liveWindow.centreWithSize(liveEditor->getWidth(),liveEditor->getHeight());liveWindow.setVisible(true);pump(50);
  juce::AudioProcessorParameter* mix=nullptr;
  for(auto* parameter:plugin->getParameters())if(parameter->getName(128).containsIgnoreCase("mix")){mix=parameter;break;}
  check(mix!=nullptr,"host exposes Mix automation");
  std::atomic<bool> done {false},finite {true};
  std::thread audioThread([&]{
   juce::AudioBuffer<float> audio(2,64);juce::MidiBuffer midi;
   for(int block=0;block<300;++block){audio.clear();audio.setSample(0,0,.01f);plugin->processBlock(audio,midi);
    for(int ch=0;ch<2;++ch)for(int i=0;i<64;++i)if(!std::isfinite(audio.getSample(ch,i)))finite=false;}
   done=true;
  });
  int automationStep=0;
  while(!done.load()){
   mix->beginChangeGesture();mix->setValueNotifyingHost(float(automationStep%100)/100.f);mix->endChangeGesture();
   if(automationStep%4==0)plugin->setCurrentProgram(automationStep%200);
   ++automationStep;pump(2);
  }
  audioThread.join();check(finite.load(),"concurrent host audio remains finite");
  liveWindow.setVisible(false);liveWindow.clearContentComponent();liveEditor.reset();pump(20);
  plugin->releaseResources();plugin.reset();pump(20);
  stage("PASS real VST3 scan/load, all layouts, buffers/rates, native editor lifecycle, 200 programs and state");
  return 0;
 }catch(const std::exception& error){std::cerr<<"HOST ERROR: "<<error.what()<<std::endl;return 1;}
}
