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
    std::array<std::atomic<float>,4> meters {};
    using Atomic = std::atomic<float>;
    std::map<juce::String,Atomic*> values;
    std::array<std::array<Atomic*,29>,8> cached {};
    std::array<Atomic*,3> master {};
    static juce::String id(int i, const char* suffix) { return "s"+juce::String(i)+"_"+suffix; }
    static juce::AudioProcessorValueTreeState::ParameterLayout layout() {
        juce::AudioProcessorValueTreeState::ParameterLayout p;
        auto number=[&](juce::String key,juce::String name,float lo,float hi,float initial) {
            p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{key,1},name,juce::NormalisableRange<float>(lo,hi,key.endsWith("_vso")?1.f:0.f),initial));
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
                number(id(i,"mod"),name+"modulation",0,100,0);
                if(i<2) {
                    number(id(i,"time2"),name+"tap 2 ms",1,4000,180);
                    number(id(i,"feedback2"),name+"tap 2 feedback %",0,95,25);
                    toggle(id(i,"sync2"),name+"tap 2 sync");
                    number(id(i,"beats2"),name+"tap 2 beats",.125f,4,.75f);
                    number(id(i,"offset"),name+"offset %",-20,20,0);
                }
                if(i==1){toggle(id(i,"triggerEnabled"),name+"trigger mode");toggle(id(i,"trigger"),name+"trigger gate");}
                if(i==2) number(id(i,"vso"),name+"VSO (0=manual,1=7.5,2=15,3=30 IPS)",0,3,0);
                if(i==3) number(id(i,"width"),name+"tight to wide",0,100,35);
            } else {
                number(id(i,"mod"),name+"modulation",0,100,0);
                if(i==7) {number(id(i,"lowRatio"),name+"low decay ratio",.25f,2,1);number(id(i,"highRatio"),name+"high decay ratio",.25f,2,1);}
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
        const char* fields[]={"level","hp","lp","mute","solo","time","feedback","sync","beats","direct","send0","send1","send2","send3","decay","predelay","mod","time2","feedback2","sync2","beats2","offset","triggerEnabled","trigger","vso","width","lowRatio","highRatio"};
        for(int i=0;i<8;++i) for(int k=0;k<28;++k) {
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
        if(b.getNumChannels()<2 || b.getNumSamples()==0)return;
        epic::Settings s; s.input=master[0]->load();s.output=master[1]->load();s.mix=master[2]->load();
        for(int i=0;i<8;++i) {
            auto v=[&](int field){return cached[i][field]->load();};
            s.strips[i]={v(0),v(1),v(2),v(3)>.5f,v(4)>.5f};
            if(i<4) {
                s.time[i]=v(5);s.feedback[i]=v(6);s.sync[i]=v(7)>.5f;s.beats[i]=v(8);s.direct[i]=v(9)>.5f;s.modulation[i]=v(16);
                if(i<2){s.time2[i]=v(17);s.feedback2[i]=v(18);s.sync2[i]=v(19)>.5f;s.beats2[i]=v(20);s.offset[i]=v(21);}
                if(i==1){s.triggerEnabled=v(22)>.5f;s.trigger=v(23)>.5f;}
                if(i==2)s.vso=v(24);if(i==3)s.crowdWidth=v(25);
                for(int r=0;r<4;++r) s.sends[i][r]=v(10+r);
            } else {s.decay[i-4]=v(14);s.predelay[i-4]=v(15);s.reverbMod[i-4]=v(16);if(i==7){s.lowRatio=v(26);s.highRatio=v(27);}}
        }
        for(int ch=0;ch<2;++ch) meters[ch].store(b.getMagnitude(ch,0,b.getNumSamples()));
        double bpm=120;
        if(auto* ph=getPlayHead()) if(auto pos=ph->getPosition()) if(auto tempo=pos->getBpm()) bpm=*tempo;
        if(b.getNumChannels()>=2) engine.process(b.getWritePointer(0),b.getWritePointer(1),b.getNumSamples(),s,bpm);
        for(int ch=0;ch<2;++ch) meters[ch+2].store(b.getMagnitude(ch,0,b.getNumSamples()));
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
                set(id(i,"beats"),s.beats[i]);set(id(i,"direct"),s.direct[i]?1.f:0.f);set(id(i,"mod"),s.modulation[i]);
                if(i<2){set(id(i,"time2"),s.time2[i]);set(id(i,"feedback2"),s.feedback2[i]);set(id(i,"sync2"),s.sync2[i]?1.f:0.f);set(id(i,"beats2"),s.beats2[i]);set(id(i,"offset"),s.offset[i]);}
                if(i==1){set(id(i,"triggerEnabled"),s.triggerEnabled?1.f:0.f);set(id(i,"trigger"),s.trigger?1.f:0.f);}
                if(i==2)set(id(i,"vso"),s.vso);if(i==3)set(id(i,"width"),s.crowdWidth);
                for(int r=0;r<4;++r) set(id(i,("send"+juce::String(r)).toRawUTF8()),s.sends[i][r]);
            } else {set(id(i,"decay"),s.decay[i-4]);set(id(i,"predelay"),s.predelay[i-4]);set(id(i,"mod"),s.reverbMod[i-4]);if(i==7){set(id(i,"lowRatio"),s.lowRatio);set(id(i,"highRatio"),s.highRatio);}}
        }
        currentProgram.store(index);
    }
    void changeProgramName(int,const juce::String&) override {}
    bool hasEditor() const override {return true;} juce::AudioProcessorEditor* createEditor() override;
    void getStateInformation(juce::MemoryBlock& dest) override { auto tree=state.copyState();tree.setProperty("factoryProgram",currentProgram.load(),nullptr);auto xml=tree.createXml();copyXmlToBinary(*xml,dest); }
    void setStateInformation(const void* data,int size) override { if(auto xml=getXmlFromBinary(data,size)) if(xml->hasTagName("EpicLab")) {auto tree=juce::ValueTree::fromXml(*xml);state.replaceState(tree);currentProgram.store(juce::jlimit(0,99,int(tree.getProperty("factoryProgram",0))));} }
};
#include "Editor.h"
juce::AudioProcessorEditor* Processor::createEditor(){return new Editor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Processor;}
