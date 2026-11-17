#include <juce_audio_utils/juce_audio_utils.h>
#include <BinaryData.h>
#include "Dsp.h"
#include "Presets.h"
#include <map>

class Processor final : public juce::AudioProcessor {
public:
    juce::AudioProcessorValueTreeState state;
    epic::Engine engine;
    std::atomic<int> currentProgram {0};
    using Atomic = std::atomic<float>;
    std::map<juce::String,Atomic*> values;
    std::array<std::array<Atomic*,18>,8> cached {};
    std::array<Atomic*,3> master {};
    static juce::String id(int i, const char* suffix) { return "s"+juce::String(i)+"_"+suffix; }
    static juce::AudioProcessorValueTreeState::ParameterLayout layout() {
        juce::AudioProcessorValueTreeState::ParameterLayout p;
        auto number=[&](juce::String key,juce::String name,float lo,float hi,float initial) {
            p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{key,1},name,juce::NormalisableRange<float>(lo,hi),initial));
        };
        auto toggle=[&](juce::String key,juce::String name,bool initial=false) {
            p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{key,1},name,initial));
        };
        number("input","Input dB",-24,24,0); number("output","Output dB",-24,12,0); number("mix","Mix %",0,100,30);
        const char* names[]={"Tape","Throw","Slap","Crowd","Plate","Room","Hall","Space"};
        const float times[]={120,250,100,35};
        for(int i=0;i<8;++i) {
            auto name=juce::String(names[i])+" ";
            number(id(i,"level"),name+"level dB",-60,6,-18);
            number(id(i,"hp"),name+"HP Hz",20,2000,20);
            number(id(i,"lp"),name+"LP Hz",1000,20000,20000);
            toggle(id(i,"mute"),name+"mute"); toggle(id(i,"solo"),name+"solo");
            if(i<4) {
                number(id(i,"time"),name+"time ms",1,4000,times[i]);
                number(id(i,"feedback"),name+"feedback %",0,95,25);
                toggle(id(i,"sync"),name+"tempo sync");
                number(id(i,"beats"),name+"quarter-note beats",.125f,4,.5f);
                toggle(id(i,"direct"),name+"direct output",true);
                for(int r=0;r<4;++r) number(id(i,("send"+juce::String(r)).toRawUTF8()),name+"send to "+names[r+4]+" %",0,100,0);
            } else {
                number(id(i,"decay"),name+"decay seconds",.1f,10,1.5f);
                number(id(i,"predelay"),name+"predelay ms",0,500,0);
            }
        }
        return p;
    }
    Processor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),
        state(*this,nullptr,"EpicLab",layout()) {
        for(auto* p:getParameters()) if(auto* named=dynamic_cast<juce::AudioProcessorParameterWithID*>(p)) values[named->paramID]=state.getRawParameterValue(named->paramID);
        master={values.at("input"),values.at("output"),values.at("mix")};
        const char* fields[]={"level","hp","lp","mute","solo","time","feedback","sync","beats","direct","send0","send1","send2","send3","decay","predelay"};
        for(int i=0;i<8;++i) for(int k=0;k<16;++k) {
            auto found=values.find(id(i,fields[k]));
            if(found!=values.end()) cached[i][k]=found->second;
        }
        setCurrentProgram(0);
    }
    float value(const juce::String& key) const { return values.at(key)->load(); }
    const juce::String getName() const override { return "EpicLab"; }
    void prepareToPlay(double sr,int) override { engine.prepare(sr); }
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& b) const override { return b.getMainInputChannelSet()==juce::AudioChannelSet::stereo() && b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo(); }
    void processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&) override {
        juce::ScopedNoDenormals guard;
        epic::Settings s; s.input=master[0]->load();s.output=master[1]->load();s.mix=master[2]->load();
        for(int i=0;i<8;++i) {
            auto v=[&](int field){return cached[i][field]->load();};
            s.strips[i]={v(0),v(1),v(2),v(3)>.5f,v(4)>.5f};
            if(i<4) {
                s.time[i]=v(5);s.feedback[i]=v(6);s.sync[i]=v(7)>.5f;s.beats[i]=v(8);s.direct[i]=v(9)>.5f;
                for(int r=0;r<4;++r) s.sends[i][r]=v(10+r);
            } else {s.decay[i-4]=v(14);s.predelay[i-4]=v(15);}
        }
        double bpm=120;
        if(auto* ph=getPlayHead()) if(auto pos=ph->getPosition()) if(auto tempo=pos->getBpm()) bpm=*tempo;
        if(b.getNumChannels()>=2) engine.process(b.getWritePointer(0),b.getWritePointer(1),b.getNumSamples(),s,bpm);
    }
    bool acceptsMidi() const override {return false;} bool producesMidi() const override {return false;}
    double getTailLengthSeconds() const override {return 90;}
    int getNumPrograms() override {return 100;}
    int getCurrentProgram() override {return currentProgram.load();}
    const juce::String getProgramName(int index) override {
        return index>=0 && index<100 ? juce::String(epic::factoryPresets()[size_t(index)].name) : juce::String();
    }
    void setCurrentProgram(int index) override {
        if(index<0 || index>=100) return;
        const auto& s=epic::factoryPresets()[size_t(index)].settings;
        auto set=[&](const juce::String& key,float value) {
            if(auto* parameter=state.getParameter(key)) {
                parameter->beginChangeGesture();
                parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
                parameter->endChangeGesture();
            }
        };
        set("input",s.input);set("output",s.output);set("mix",s.mix);
        for(int i=0;i<8;++i) {
            const auto& strip=s.strips[i];
            set(id(i,"level"),strip.level);set(id(i,"hp"),strip.hp);set(id(i,"lp"),strip.lp);
            set(id(i,"mute"),strip.mute?1.f:0.f);set(id(i,"solo"),strip.solo?1.f:0.f);
            if(i<4) {
                set(id(i,"time"),s.time[i]);set(id(i,"feedback"),s.feedback[i]);set(id(i,"sync"),s.sync[i]?1.f:0.f);
                set(id(i,"beats"),s.beats[i]);set(id(i,"direct"),s.direct[i]?1.f:0.f);
                for(int r=0;r<4;++r) set(id(i,("send"+juce::String(r)).toRawUTF8()),s.sends[i][r]);
            } else {set(id(i,"decay"),s.decay[i-4]);set(id(i,"predelay"),s.predelay[i-4]);}
        }
        currentProgram.store(index);
    }
    void changeProgramName(int,const juce::String&) override {}
    bool hasEditor() const override {return true;} juce::AudioProcessorEditor* createEditor() override;
    void getStateInformation(juce::MemoryBlock& dest) override { auto tree=state.copyState();tree.setProperty("factoryProgram",currentProgram.load(),nullptr);auto xml=tree.createXml();copyXmlToBinary(*xml,dest); }
    void setStateInformation(const void* data,int size) override { if(auto xml=getXmlFromBinary(data,size)) if(xml->hasTagName("EpicLab")) {auto tree=juce::ValueTree::fromXml(*xml);state.replaceState(tree);currentProgram.store(juce::jlimit(0,99,int(tree.getProperty("factoryProgram",0))));} }
};
class Editor final : public juce::AudioProcessorEditor, private juce::Timer {
    Processor& processor;
    juce::ComboBox presets;
    void timerCallback() override {presets.setSelectedId(processor.getCurrentProgram()+1,juce::dontSendNotification);}
    juce::Image image=juce::ImageCache::getFromMemory(BinaryData::imagem_png,BinaryData::imagem_pngSize);
    juce::TextButton controls {"Abrir controles de audio"};
    juce::GenericAudioProcessorEditor parameters;
public:
    explicit Editor(Processor& p):AudioProcessorEditor(p),processor(p),parameters(p) {
        addAndMakeVisible(presets);
        for(int i=0;i<p.getNumPrograms();++i) presets.addItem(p.getProgramName(i),i+1);
        presets.setSelectedId(p.getCurrentProgram()+1,juce::dontSendNotification);
        presets.onChange=[this]{processor.setCurrentProgram(presets.getSelectedId()-1);};
        startTimerHz(5);
        addAndMakeVisible(controls);addChildComponent(parameters);
        controls.onClick=[this]{parameters.setVisible(!parameters.isVisible());controls.setButtonText(parameters.isVisible()?"Ver imagem de referencia":"Abrir controles de audio");repaint();};
        setSize(1000,680);setResizable(true,true);setResizeLimits(640,480,1800,1200);
    }
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colours::black);
        if(!parameters.isVisible()) g.drawImageWithin(image,0,40,getWidth(),getHeight()-40,juce::RectanglePlacement::centred);
    }
    void resized() override {controls.setBounds(10,5,260,30);presets.setBounds(280,5,getWidth()-290,30);parameters.setBounds(0,40,getWidth(),getHeight()-40);}
};
juce::AudioProcessorEditor* Processor::createEditor(){return new Editor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Processor;}
