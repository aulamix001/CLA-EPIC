#pragma once
// Live JUCE components over the immutable reference image. No raster edits.
class PanelLook final : public juce::LookAndFeel_V4 {
public:
    PanelLook() {
        setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xffc7ebff));
        setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff07151d));
        setColour(juce::Slider::textBoxOutlineColourId,juce::Colour(0xff365565));
        setColour(juce::Label::textColourId,juce::Colour(0xffc4cbd0));
        setColour(juce::TextButton::buttonColourId,juce::Colour(0xff263038));
        setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff246e87));
    }
    void drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float,float,
                          juce::Slider::SliderStyle style,juce::Slider& slider) override {
        if(style!=juce::Slider::LinearVertical){juce::LookAndFeel_V4::drawLinearSlider(g,x,y,w,h,pos,0,0,style,slider);return;}
        const float cx=float(x+w/2);
        g.setColour(juce::Colour(0xff090c0e));g.fillRoundedRectangle(cx-4,float(y),8,float(h),3);
        g.setColour(juce::Colour(0xff77838b));
        for(int i=0;i<=6;++i)g.drawHorizontalLine(y+i*h/6,cx+9,cx+17);
        juce::Rectangle<float> thumb(cx-18,pos-14,36,28);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xffbec6cb),0,pos-14,juce::Colour(0xff444a50),0,pos+14,false));
        g.fillRoundedRectangle(thumb,2);
        g.setColour(slider.findColour(juce::Slider::thumbColourId));g.fillRect(cx-17,pos-1,34.f,2.f);
    }
};
class BoundSlider final : public juce::Component {
public:
    juce::Label label;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    BoundSlider(Processor& p,const juce::String& id,const juce::String& title,bool fader=false) {
        label.setText(title,juce::dontSendNotification);label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);addAndMakeVisible(slider);slider.setComponentID(id);
        slider.setName(title);slider.setTooltip(p.state.getParameter(id)->getName(120));
        slider.setSliderStyle(fader?juce::Slider::LinearVertical:juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(fader?juce::Slider::TextBoxAbove:juce::Slider::TextBoxBelow,false,fader?100:92,22);
        slider.setNumDecimalPlacesToDisplay(id.endsWith("hp")||id.endsWith("lp")?0:2);
        slider.setDoubleClickReturnValue(true,p.state.getParameter(id)->convertFrom0to1(p.state.getParameter(id)->getDefaultValue()));
        attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,id,slider);
    }
    void resized() override {label.setBounds(0,0,getWidth(),18);slider.setBounds(0,18,getWidth(),getHeight()-18);}
};
class BoundButton final : public juce::TextButton {
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
public:
    BoundButton(Processor& p,const juce::String& id,const juce::String& text):juce::TextButton(text) {
        setComponentID(id);setClickingTogglesState(true);setTooltip(p.state.getParameter(id)->getName(120));
        attachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,id,*this);
    }
};
class LivePanel final : public juce::Component {
    Processor& processor;
    juce::Colour accent;
    int selected=0;
    bool reverb;
    std::array<juce::TextButton,4> tabs;
    std::vector<std::unique_ptr<BoundSlider>> knobs;
    std::vector<std::unique_ptr<BoundButton>> buttons;
    void rebuild() {
        knobs.clear();buttons.clear();
        const int i=selected+(reverb?4:0);
        auto knob=[&](const char* field,const char* text){
            auto c=std::make_unique<BoundSlider>(processor,Processor::id(i,field),text);
            c->slider.setColour(juce::Slider::rotarySliderFillColourId,accent);
            c->slider.setColour(juce::Slider::thumbColourId,accent);addAndMakeVisible(*c);knobs.push_back(std::move(c));
        };
        auto button=[&](const char* field,const char* text){auto c=std::make_unique<BoundButton>(processor,Processor::id(i,field),text);addAndMakeVisible(*c);buttons.push_back(std::move(c));};
        if(reverb) {
            knob("predelay","PRE DELAY ms");knob("decay","DECAY sec");knob("mod","MODULATION");
            if(i==7){knob("lowRatio","LOW DECAY x");knob("highRatio","HIGH DECAY x");}
        } else {
            if(i<4){knob("time","TAP 1 ms");knob("feedback","FEEDBACK 1 %");knob("beats","BEATS 1");}
            if(i<2){knob("time2","TAP 2 ms");knob("feedback2","FEEDBACK 2 %");knob("beats2","BEATS 2");knob("offset","OFFSET %");}
            if(i==2)knob("vso","VSO 0/7.5/15/30");
            if(i==3)knob("width","TIGHT / WIDE");
            knob("mod","MODULATION");
            if(i<4)button("sync","SYNC 1");if(i<2)button("sync2","SYNC 2");
            if(i==1){button("triggerEnabled","TRIGGER MODE");button("trigger","GATE OPEN");}
            button("direct","DIRECT OUT");
            knob("send0","SEND A %");knob("send1","SEND B %");knob("send2","SEND C %");knob("send3","SEND D %");
        }
        for(int n=0;n<4;++n)tabs[n].setToggleState(n==selected,juce::dontSendNotification);
        resized();repaint();
    }
public:
    LivePanel(Processor& p,bool rev):processor(p),accent(rev?juce::Colour(0xffff624c):juce::Colour(0xff61ceff)),reverb(rev) {
        const char* names[]={"TAPE","THROW","SLAP","CROWD","PLATE","ROOM","HALL","SPACE"};
        for(int n=0;n<4;++n){tabs[n].setButtonText(names[n+(rev?4:0)]);tabs[n].setComponentID(juce::String(rev?"reverbTab":"delayTab")+juce::String(n));tabs[n].onClick=[this,n]{select(n);};addAndMakeVisible(tabs[n]);}
        rebuild();
    }
    void select(int n){if(n>=0&&n<4){selected=n;rebuild();}}
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff0b1319));g.setColour(accent.withAlpha(.6f));g.drawRect(getLocalBounds(),2);
        g.setColour(accent);g.setFont(14.f);
        if(!reverb)g.drawText("DELAY > REVERB ROUTING",8,246,getWidth()-16,18,juce::Justification::centred);
        else g.drawText("Live parameters - automation and presets",8,286,getWidth()-16,24,juce::Justification::centred);
    }
    void resized() override {
        const int w=getWidth();for(int n=0;n<4;++n)tabs[n].setBounds(8+n*(w-16)/4,8,(w-16)/4-4,30);
        const int mainCount=int(knobs.size())-(reverb?0:4);
        for(int n=0;n<mainCount;++n)knobs[size_t(n)]->setBounds(8+(n%4)*(w-16)/4,46+(n/4)*82,(w-16)/4-4,80);
        for(size_t n=0;n<buttons.size();++n)buttons[n]->setBounds(8+int(n)*(w-16)/int(buttons.size()),216,(w-16)/int(buttons.size())-4,26);
        if(!reverb)for(int n=0;n<4;++n)knobs[size_t(mainCount+n)]->setBounds(8+n*(w-16)/4,268,(w-16)/4-4,76);
    }
};
class LiveSurface final : public juce::Component, private juce::Timer {
    Processor& processor;
    PanelLook look;
    juce::Image image=juce::ImageCache::getFromMemory(BinaryData::imagem_png,BinaryData::imagem_pngSize);
    std::array<std::unique_ptr<BoundSlider>,8> level,hp,lp;
    std::array<std::unique_ptr<BoundButton>,8> mute,solo,quickMute;
    std::array<juce::TextButton,8> select;
    BoundSlider input,output,mix;
    LivePanel delays,reverbs;
    std::array<float,4> meterValues {};
    void timerCallback() override {
        for(int i=0;i<4;++i)meterValues[i]=std::max(processor.meters[i].load(),meterValues[i]*.85f);
        repaint(25,25,110,185);repaint(1435,25,110,185);
    }
public:
    explicit LiveSurface(Processor& p):processor(p),input(p,"input","INPUT dB",true),output(p,"output","OUTPUT dB",true),mix(p,"mix","MIX %"),delays(p,false),reverbs(p,true) {
        setLookAndFeel(&look);
        addAndMakeVisible(input);addAndMakeVisible(output);addAndMakeVisible(mix);addAndMakeVisible(delays);addAndMakeVisible(reverbs);
        const char* names[]={"1 TAPE","2 THROW","3 SLAP","4 CROWD","A PLATE","B ROOM","C HALL","D SPACE"};
        for(int i=0;i<8;++i) {
            level[i]=std::make_unique<BoundSlider>(p,Processor::id(i,"level"),"LEVEL dB",true);
            hp[i]=std::make_unique<BoundSlider>(p,Processor::id(i,"hp"),"HP Hz");
            lp[i]=std::make_unique<BoundSlider>(p,Processor::id(i,"lp"),"LP Hz");
            mute[i]=std::make_unique<BoundButton>(p,Processor::id(i,"mute"),"M");solo[i]=std::make_unique<BoundButton>(p,Processor::id(i,"solo"),"S");
            quickMute[i]=std::make_unique<BoundButton>(p,Processor::id(i,"mute"),juce::String(i<4?i+1:i-3));
            addAndMakeVisible(*level[i]);addAndMakeVisible(*hp[i]);addAndMakeVisible(*lp[i]);addAndMakeVisible(*mute[i]);addAndMakeVisible(*solo[i]);addAndMakeVisible(*quickMute[i]);
            const auto colour=i<4?juce::Colour(0xff61ceff):juce::Colour(0xffff624c);
            level[i]->slider.setColour(juce::Slider::thumbColourId,colour);
            hp[i]->slider.setColour(juce::Slider::rotarySliderFillColourId,colour);lp[i]->slider.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xffd85b54));
            select[i].setButtonText(names[i]);select[i].setComponentID("stripSelect"+juce::String(i));
            select[i].onClick=[this,i]{if(i<4)delays.select(i);else reverbs.select(i-4);};addAndMakeVisible(select[i]);
        }
        startTimerHz(25);
    }
    ~LiveSurface() override {setLookAndFeel(nullptr);}
    void paint(juce::Graphics& g) override {
        g.drawImage(image,getLocalBounds().toFloat());
        // Hide only the static controls at draw time; original PNG bytes are untouched.
        g.setColour(juce::Colour(0xff24292d));g.fillRect(174,394,1228,572);
        g.fillRect(20,267,123,181);g.fillRect(28,592,116,373);g.fillRect(1419,374,128,592);
        g.setColour(juce::Colour(0xff10171c));g.fillRect(30,28,110,165);g.fillRect(1438,28,110,165);
        for(int i=0;i<8;++i){const int x=i<4?176+i*150:802+(i-4)*150;g.setColour(juce::Colour(0xff41494e));g.drawRect(x,394,148,566);}
        for(int side=0;side<2;++side)for(int ch=0;ch<2;++ch)for(int led=0;led<10;++led){
            const float db=juce::Decibels::gainToDecibels(meterValues[size_t(side*2+ch)],-60.f);
            const bool on=db>=-54+led*6;
            g.setColour(on?(led>7?juce::Colours::orangered:juce::Colours::limegreen):juce::Colour(0xff253229));
            g.fillRoundedRectangle(float((side==0?58:1460)+ch*27),float(173-led*14),16.f,8.f,2.f);
        }
    }
    void resized() override {
        delays.setBounds(180,30,586,350);reverbs.setBounds(806,30,586,350);
        input.setBounds(32,596,106,358);output.setBounds(1428,596,106,358);mix.setBounds(1428,385,106,153);
        for(int i=0;i<8;++i){
            const int x=i<4?176+i*150:802+(i-4)*150;
            hp[i]->setBounds(x+5,406,68,122);lp[i]->setBounds(x+75,406,68,122);
            solo[i]->setBounds(x+24,536,43,28);mute[i]->setBounds(x+80,536,43,28);
            level[i]->setBounds(x+13,576,122,336);select[i].setBounds(x+4,924,140,30);
            quickMute[i]->setBounds(32+(i/4)*57,277+(i%4)*41,50,35);
        }
    }
};
class Editor final : public juce::AudioProcessorEditor, private juce::Timer {
    Processor& processor;
    juce::ComboBox presets;
    juce::TextButton previous {"<"},next {">"};
    LiveSurface surface;
    juce::TooltipWindow tooltips {this,650};
    void timerCallback() override {presets.setSelectedId(processor.getCurrentProgram()+1,juce::dontSendNotification);}
public:
    explicit Editor(Processor& p):AudioProcessorEditor(p),processor(p),surface(p) {
        presets.setComponentID("factoryPresets");addAndMakeVisible(presets);addAndMakeVisible(previous);addAndMakeVisible(next);addAndMakeVisible(surface);
        for(int i=0;i<p.getNumPrograms();++i)presets.addItem(p.getProgramName(i),i+1);
        presets.setSelectedId(p.getCurrentProgram()+1,juce::dontSendNotification);
        presets.onChange=[this]{processor.setCurrentProgram(presets.getSelectedId()-1);};
        previous.onClick=[this]{processor.setCurrentProgram((processor.getCurrentProgram()+99)%100);timerCallback();};
        next.onClick=[this]{processor.setCurrentProgram((processor.getCurrentProgram()+1)%100);timerCallback();};
        surface.setBounds(0,0,1568,1008);
        setSize(1100,748);setResizable(true,true);setResizeLimits(900,620,1882,1250);startTimerHz(10);
    }
    void paint(juce::Graphics& g) override {g.fillAll(juce::Colour(0xff101418));}
    void resized() override {
        previous.setBounds(12,6,40,28);presets.setBounds(60,6,getWidth()-120,28);next.setBounds(getWidth()-52,6,40,28);
        const float scale=std::min(float(getWidth())/1568.f,float(getHeight()-44)/1008.f);
        surface.setTransform(juce::AffineTransform::scale(scale).translated((getWidth()-1568*scale)*.5f,44));
    }
};
