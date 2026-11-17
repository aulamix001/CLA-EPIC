#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <thread>
#include <mutex>
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
struct CapturePreset final : juce::ExtensionsVisitor {
 juce::MemoryBlock data;
 void visitVST3Client(const VST3Client& client) override {data=client.getPreset();}
};
struct RestorePreset final : juce::ExtensionsVisitor {
 const juce::MemoryBlock& data;bool restored=false;
 explicit RestorePreset(const juce::MemoryBlock& bytes):data(bytes){}
 void visitVST3Client(const VST3Client& client) override {restored=client.setPreset(data);}
};
juce::MemoryBlock componentChunk(const juce::MemoryBlock& preset){
 check(preset.getSize()>=48&&std::memcmp(preset.getData(),"VST3",4)==0,"native preset VST3 header");
 juce::MemoryInputStream stream(preset,false);stream.setPosition(40);const auto list=stream.readInt64();
 check(list>=48&&list<=juce::int64(preset.getSize())-8,"preset chunk list offset");stream.setPosition(list);
 char tag[4];stream.read(tag,4);check(std::memcmp(tag,"List",4)==0,"native preset chunk table");const int count=stream.readInt();check(count>0&&count<=16,"preset chunk count");
 for(int i=0;i<count;++i){stream.read(tag,4);const auto offset=stream.readInt64(),size=stream.readInt64();
  check(offset>=48&&size>=0&&offset<=juce::int64(preset.getSize())&&size<=juce::int64(preset.getSize())-offset,"preset chunk bounds");
  if(std::memcmp(tag,"Comp",4)==0)return {static_cast<const char*>(preset.getData())+offset,size_t(size)};
 }
 throw std::runtime_error("preset contains no component state");
}
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
    const auto presetDirectory=juce::File::getCurrentWorkingDirectory().getChildFile("factory-presets");
    check(presetDirectory.createDirectory().wasOk(),"create preset export directory");juce::Array<juce::var> manifest;
    for(int preset=0;preset<200;++preset){
     plugin->setCurrentProgram(preset);check(plugin->getCurrentProgram()==preset,"VST3 program change");
     audio.clear();plugin->processBlock(audio,midi);
     CapturePreset capture;plugin->getExtensions(capture);const auto component=componentChunk(capture.data);
     const auto name=plugin->getProgramName(preset),group=name.upToFirstOccurrenceOf(" / ",false,false);
     const auto folder=presetDirectory.getChildFile(juce::File::createLegalFileName(group));check(folder.createDirectory().wasOk(),"create preset category");
     const auto filename=juce::File::createLegalFileName(juce::String(preset+1).paddedLeft('0',3)+" "+name.replace(" / "," - ")+".vstpreset");
     const auto file=folder.getChildFile(filename);check(file.replaceWithData(capture.data.getData(),capture.data.getSize()),"export native vstpreset");
     plugin->setCurrentProgram((preset+1)%200);audio.clear();plugin->processBlock(audio,midi);
     CapturePreset different;plugin->getExtensions(different);check(componentChunk(different.data)!=component,"preset roundtrip starts from a different state");
     juce::MemoryBlock fromDisk;check(file.loadFileAsData(fromDisk),"read exported preset");RestorePreset restore(fromDisk);plugin->getExtensions(restore);check(restore.restored,"import native vstpreset");
     CapturePreset roundTrip;plugin->getExtensions(roundTrip);check(componentChunk(roundTrip.data)==component,"all preset component settings survive file roundtrip");
     juce::var item(new juce::DynamicObject());auto* entry=item.getDynamicObject();entry->setProperty("number",preset+1);entry->setProperty("name",name);entry->setProperty("file",file.getRelativePathFrom(presetDirectory).replaceCharacter(juce::File::getSeparatorChar(),'/'));manifest.add(item);
     if(preset%25==0){std::cout<<"HOST preset "<<preset<<std::endl;pump(2);}
    }
    check(manifest.size()==200,"200 native preset exports");check(presetDirectory.getChildFile("presets.json").replaceWithText(juce::JSON::toString(juce::var(manifest))),"write native preset manifest");
    // Synchronise the host's cached program parameter before its normal state roundtrip.
    plugin->setCurrentProgram(199);
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
  plugin->releaseResources();plugin.reset();pump(250);
  stage("remove active instances while transport remains running");
  for(int removal=0;removal<5;++removal){
   auto removing=formats.createPluginInstance(*types[0],48000,64,error);check(removing!=nullptr,"removal test creates instance");
   removing->prepareToPlay(48000,64);
   std::unique_ptr<juce::AudioProcessorEditor> editor(removing->createEditorIfNeeded());check(editor!=nullptr,"removal test editor opens");
   Window window;window.setContentNonOwned(editor.get(),true);window.centreWithSize(editor->getWidth(),editor->getHeight());window.setVisible(true);pump(75);
   std::atomic<bool> stop {false};std::mutex graphLock;auto* graphNode=removing.get();
   std::thread transport([&]{
    juce::AudioBuffer<float> audio(2,64);juce::MidiBuffer midi;
    while(!stop.load()){
     {std::lock_guard<std::mutex> guard(graphLock);if(graphNode){audio.clear();graphNode->processBlock(audio,midi);}}
     std::this_thread::yield();
    }
   });
   // A DAW detaches the node under its graph lock before destroying it;
   // the transport itself continues while queued UI callbacks are drained.
   pump(40);
   {std::lock_guard<std::mutex> guard(graphLock);graphNode=nullptr;}
   window.setVisible(false);window.clearContentComponent();editor.reset();
   removing->releaseResources();removing.reset();pump(250);
   stop=true;transport.join();std::cout<<"HOST removal "<<removal<<" completed"<<std::endl;
  }
  stage("PASS real VST3 scan/load, all layouts, buffers/rates, native editor lifecycle, 200 programs and state");
  return 0;
 }catch(const std::exception& error){std::cerr<<"HOST ERROR: "<<error.what()<<std::endl;return 1;}
}
