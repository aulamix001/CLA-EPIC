#pragma once
// The original PNG is the skin. Only existing control footprints are repainted.
namespace skin {
inline juce::Colour blue(){return juce::Colour(0xff8ddfff);}
inline juce::Colour red(){return juce::Colour(0xffff492a);}
inline juce::Image original(){
    // Retain only CPU pixels. Native/Direct2D images must not survive until DLL unload.
    static const auto cpuImage=[] {
        auto decoded=juce::ImageFileFormat::loadFrom(BinaryData::imagem_png,size_t(BinaryData::imagem_pngSize));
        return juce::SoftwareImageType{}.convert(decoded);
    }();
    return cpuImage;
}
inline int stripX(int i){return i<4?174+i*150:800+(i-4)*150;}
inline void digital(juce::Graphics& g,juce::String text,juce::Rectangle<float> area,juce::Colour colour){
    // Fourteen-segment strokes match the display language instead of native widgets.
    static const float lines[14][4]={{3,1,17,1},{19,3,18,15},{17,19,16,31},{2,33,15,33},{0,19,1,31},{2,3,1,15},{2,17,9,17},{10,17,17,17},{3,3,8,15},{11,3,10,15},{16,3,11,15},{2,31,8,19},{9,31,9,19},{15,31,11,19}};
    auto bits=[](std::initializer_list<int> list){unsigned b=0;for(int n:list)b|=1u<<n;return b;};
    auto glyph=[&](juce::juce_wchar c)->unsigned {
        switch(c){
        case '0':case 'O':return bits({0,1,2,3,4,5});case '1':return bits({1,2});case '2':return bits({0,1,6,7,4,3});
        case '3':return bits({0,1,2,3,6,7});case '4':return bits({5,6,7,1,2});case '5':case 'S':return bits({0,5,6,7,2,3});
        case '6':return bits({0,5,4,3,2,6,7});case '7':return bits({0,1,2});case '8':case 'B':return bits({0,1,2,3,4,5,6,7});case '9':return bits({0,1,2,3,5,6,7});
        case 'A':return bits({0,1,2,4,5,6,7});case 'C':return bits({0,3,4,5});case 'D':return bits({0,1,2,3,9,12});case 'E':return bits({0,3,4,5,6,7});
        case 'F':return bits({0,4,5,6,7});case 'G':return bits({0,2,3,4,5,7});case 'H':return bits({1,2,4,5,6,7});case 'I':return bits({0,3,9,12});
        case 'L':return bits({3,4,5});case 'M':return bits({1,2,4,5,8,10});case 'N':return bits({1,2,4,5,8,13});case 'P':return bits({0,1,4,5,6,7});
        case 'R':return bits({0,1,4,5,6,7,13});case 'T':return bits({0,9,12});case 'U':return bits({1,2,3,4,5});case 'V':return bits({4,5,10,11});
        case 'W':return bits({1,2,4,5,11,13});case 'Y':return bits({8,10,12});case '-':return bits({6,7});case '/':return bits({10,11});default:return 0;
        }
    };
    text=text.toUpperCase();float width=0;for(auto c:text)width+=c=='.'?6.f:24.f;width=std::max(1.f,width);
    const float scale=std::min(area.getWidth()/width,area.getHeight()/35.f);
    const float start=area.getX()+(area.getWidth()-width*scale)*.5f, top=area.getCentreY()-17.f*scale;
    float advance=0;for(auto c:text){const float x=start+advance*scale;advance+=c=='.'?6.f:24.f;auto mask=glyph(c);
        if(c!='.')for(int n=0;n<14;++n){g.setColour(colour.withAlpha((mask&(1u<<n))?.95f:.035f));const auto* l=lines[n];g.drawLine(x+l[0]*scale,top+l[1]*scale,x+l[2]*scale,top+l[3]*scale,std::max(1.5f,2.4f*scale));}
        if(c=='.'){g.setColour(colour);g.fillEllipse(x+1*scale,top+30*scale,3*scale,3*scale);}
    }
}
inline void display(juce::Graphics& g,juce::Rectangle<int> rect,const juce::String& value,juce::Colour c,bool grid=true){
    if(grid){
        static const auto blueTile=original().getClippedImage({220,249,16,8});
        static const auto redTile=original().getClippedImage({835,348,16,8});
        g.setTiledImageFill(c==red()?redTile:blueTile,float(rect.getX()),float(rect.getY()),1.f);g.fillRect(rect);
    }else{g.setColour(juce::Colour(0xff071017));g.fillRect(rect);}
    digital(g,value,rect.toFloat().reduced(2),c);
}
}
class SkinSlider final : public juce::Slider {
public:
    enum Kind {number,knob,fader,route};
private:
    Kind kind;juce::Rectangle<int> source,thumb;juce::Image image=skin::original();juce::Colour colour;
    juce::String routeLetter;float emptyX=0;juce::Point<int> down;double downValue=0;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
public:
    SkinSlider(Processor& p,const juce::String& id,Kind k,juce::Rectangle<int> rect,juce::Colour c,juce::Rectangle<int> sprite={},float cleanX=0,juce::String letter={})
      :kind(k),source(rect),thumb(sprite),colour(c),routeLetter(letter),emptyX(cleanX){
        setComponentID(id);setBounds(rect);setName(p.state.getParameter(id)->getName(120));setTooltip(getName()+" - arraste ou use a roda");
        setSliderStyle(k==fader?LinearVertical:k==knob?RotaryHorizontalVerticalDrag:LinearHorizontal);setTextBoxStyle(NoTextBox,false,0,0);
        setMouseDragSensitivity(k==number?180:300);setScrollWheelEnabled(true);setPopupDisplayEnabled(true,true,nullptr);
        attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,id,*this);
        setDoubleClickReturnValue(true,p.state.getParameter(id)->convertFrom0to1(p.state.getParameter(id)->getDefaultValue()));
    }
    double valueToProportionOfLength(double value) override {
        if(kind!=fader)return juce::Slider::valueToProportionOfLength(value);
        if(getComponentID()=="input"||getComponentID()=="output")return juce::jlimit(0.0,1.0,(value+12.0)/24.0);
        const double y=value>=-40?655.0-value*4.65:841.0+(-40-value)*2.25;
        return juce::jlimit(0.0,1.0,(886.0-y)/276.0);
    }
    double proportionOfLengthToValue(double proportion) override {
        if(kind!=fader)return juce::Slider::proportionOfLengthToValue(proportion);
        if(getComponentID()=="input"||getComponentID()=="output")return proportion*24.0-12.0;
        const double y=886.0-proportion*276.0;
        return juce::jlimit(getMinimum(),getMaximum(),y<=841?(655.0-y)/4.65:-40.0-(y-841.0)/2.25);
    }
    void mouseDown(const juce::MouseEvent& e) override {down=e.getPosition();downValue=getValue();juce::Slider::mouseDown(e);}
    void mouseDrag(const juce::MouseEvent& e) override {
        if(kind==fader)setValue(proportionOfLengthToValue(juce::jlimit(0.0,1.0,1.0-(e.position.y-32.0)/276.0)),juce::sendNotificationSync);
        else juce::Slider::mouseDrag(e);
    }
    void mouseUp(const juce::MouseEvent& e) override {
        if(kind==route && e.getPosition().getDistanceFrom(down)<3)setValue(downValue>0?0:100,juce::sendNotificationSync);
        juce::Slider::mouseUp(e);
    }
    void paint(juce::Graphics& g) override {
        const auto area=getLocalBounds();
        if(kind==knob){
            juce::Path circle;circle.addEllipse(area.toFloat().reduced(2));g.reduceClipRegion(circle);
            const auto sprite=image.getClippedImage(source);
            const float angle=float(valueToProportionOfLength(getValue()))*4.712389f;
            g.drawImageTransformed(sprite,juce::AffineTransform::rotation(angle,float(getWidth())*.5f,float(getHeight())*.5f));
        } else if(kind==fader){
            // Copy neighbouring metal texture, preserving the original thumb sprite.
            g.drawImage(image,0,0,getWidth(),getHeight(),int(emptyX),source.getY(),getComponentID()=="output"?1:getWidth(),source.getHeight());
            const int cx=getWidth()/2;
            g.setColour(juce::Colour(0xff060809));g.fillRoundedRectangle(float(cx-4),32,8.f,276.f,3.f);
            const float t=float(1-valueToProportionOfLength(getValue()));const int y=int(32+t*276)-thumb.getHeight()/2;
            g.drawImage(image,cx-thumb.getWidth()/2,y,thumb.getWidth(),thumb.getHeight(),thumb.getX(),thumb.getY(),thumb.getWidth(),thumb.getHeight());
        } else if(kind==route){
            g.setColour(getValue()>0?colour.withAlpha(.8f):juce::Colour(0xff10222b));g.fillRect(area.reduced(1));g.setColour(colour);g.drawRect(area.reduced(1));
            g.setColour(getValue()>0?juce::Colours::black:colour);g.setFont(17.f);g.drawText(routeLetter,area,juce::Justification::centred);
        } else {
            juce::String value;
            if(getComponentID().endsWith("_beats")||getComponentID().endsWith("_beats2")){
                const double beat=getValue();int denominator=int(std::round(4.0/beat));
                value=std::abs(4.0/denominator-beat)<.001?"1/"+juce::String(denominator):juce::String(beat,2);
            }else if(getComponentID().endsWith("_vso")){const char* ips[]={"OFF","7.5","15","30"};value=ips[juce::jlimit(0,3,int(getValue()))];}
            else value=juce::String(getValue(),getMaximum()>100?0:1);
            skin::display(g,area,value,colour);
        }
    }
};
class SkinButton final : public juce::TextButton {
    juce::Rectangle<int> sprite;juce::Image image=skin::original();juce::Colour colour;bool transparent;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
public:
    SkinButton(Processor& p,const juce::String& id,juce::Rectangle<int> rect,juce::Rectangle<int> source,juce::Colour c,bool invisible=false)
      :sprite(source),colour(c),transparent(invisible){
        setComponentID(id);setBounds(rect);setClickingTogglesState(true);setTooltip(p.state.getParameter(id)->getName(120));
        attachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,id,*this);
    }
    void paintButton(juce::Graphics& g,bool,bool) override {
        if(transparent)return;
        if(!sprite.isEmpty())g.drawImage(image,0,0,getWidth(),getHeight(),sprite.getX(),sprite.getY(),sprite.getWidth(),sprite.getHeight());
        const auto id=getComponentID();
        if(id.endsWith("_sync")||id.endsWith("_sync2")||id.endsWith("_triggerEnabled")){
            skin::display(g,getLocalBounds().reduced(2),getToggleState()?"ON":"OFF",getToggleState()?colour:colour.withAlpha(.5f));
        } else if(sprite.getHeight()==36){
            const int index=(sprite.getY()-275)/43+(sprite.getX()<80?0:4);
            const bool originalOn=index==1||index==2||index==6;
            if(getToggleState()!=originalOn){g.setColour(getToggleState()?colour.withAlpha(.45f):juce::Colours::black.withAlpha(.65f));g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(2),2);}
        } else if(getToggleState()){g.setColour(colour.withAlpha(.45f));g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(2),2);}
        else if(sprite.isEmpty()||id.endsWith("_direct")){g.setColour(juce::Colours::black.withAlpha(.7f));g.fillRect(getLocalBounds().reduced(1));}
    }
};
class SkinAction final : public juce::TextButton {
public:
    void paintButton(juce::Graphics&,bool,bool) override {} // Original icon is already in the skin.
};
class SkinPresets final : public juce::ComboBox {
public:
    SkinPresets(){for(auto* c:getChildren())c->setVisible(false);}
    void paint(juce::Graphics&) override {}
    void resized() override {juce::ComboBox::resized();for(auto* c:getChildren())c->setVisible(false);}
};
class LiveSurface final : public juce::Component,private juce::Timer {
    Processor& processor;juce::Image image=skin::original();
    std::vector<std::unique_ptr<SkinSlider>> common,delayFields,reverbFields;
    std::vector<std::unique_ptr<SkinButton>> buttons,delayButtons;
    std::array<SkinAction,4> delayTabs,reverbTabs;
    SkinAction delayPrevious,delayNext,reverbPrevious,reverbNext,lock;
    int delayIndex=1,reverbIndex=3;
    std::array<float,4> meterValues {};
    std::vector<std::pair<std::atomic<float>*,float>> observed;
    bool lastMixLock=false;
    SkinSlider& slider(std::vector<std::unique_ptr<SkinSlider>>& list,const juce::String& id,SkinSlider::Kind kind,juce::Rectangle<int> rect,juce::Colour colour,juce::Rectangle<int> thumb={},float cleanX=0,juce::String letter={}){
        auto c=std::make_unique<SkinSlider>(processor,id,kind,rect,colour,thumb,cleanX,letter);auto& result=*c;addAndMakeVisible(*c);list.push_back(std::move(c));return result;
    }
    void button(std::vector<std::unique_ptr<SkinButton>>& list,const juce::String& id,juce::Rectangle<int> rect,juce::Rectangle<int> source,juce::Colour colour){
        auto c=std::make_unique<SkinButton>(processor,id,rect,source,colour);addAndMakeVisible(*c);list.push_back(std::move(c));
    }
    void selectDelay(int i){
        delayIndex=i;delayFields.clear();delayButtons.clear();const auto c=skin::blue();auto id=[&](const char* f){return Processor::id(i,f);};
        slider(delayFields,id("time"),SkinSlider::number,{340,146,70,32},c);
        slider(delayFields,id("feedback"),SkinSlider::number,{290,216,72,28},c);
        slider(delayFields,id("beats"),SkinSlider::number,{311,185,44,25},c);
        button(delayButtons,id("sync"),{204,187,38,25},{204,187,38,25},c);
        slider(delayFields,id("mod"),SkinSlider::knob,{672,49,66,66},c);
        if(i<2){
            slider(delayFields,id("time2"),SkinSlider::number,{632,146,69,32},c);
            slider(delayFields,id("feedback2"),SkinSlider::number,{577,216,71,28},c);
            slider(delayFields,id("beats2"),SkinSlider::number,{596,185,43,25},c);
            slider(delayFields,id("offset"),SkinSlider::number,{493,247,58,26},c);
            button(delayButtons,id("sync2"),{490,187,38,25},{204,187,38,25},c);
        }
        if(i==1){button(delayButtons,id("triggerEnabled"),{339,115,38,18},{339,115,38,18},c);button(delayButtons,id("trigger"),{381,115,27,18},{381,115,27,18},c);}
        if(i==2)slider(delayFields,id("vso"),SkinSlider::number,{632,146,69,32},c);
        if(i==3)slider(delayFields,id("width"),SkinSlider::number,{632,146,69,32},c);
        repaint();
    }
    void selectReverb(int r){
        reverbIndex=r;reverbFields.clear();const auto c=skin::red();auto id=[&](const char* f){return Processor::id(r+4,f);};
        slider(reverbFields,id("predelay"),SkinSlider::number,{932,194,65,33},c);
        slider(reverbFields,id("decay"),SkinSlider::number,{1174,261,58,34},c);
        slider(reverbFields,id("mod"),SkinSlider::knob,{1294,49,66,66},c);
        if(r==3){slider(reverbFields,id("lowRatio"),SkinSlider::number,{1018,316,56,27},c);slider(reverbFields,id("highRatio"),SkinSlider::number,{1252,316,56,27},c);}
        repaint();
    }
    void timerCallback() override {
        bool changed=false;
        for(auto& item:observed){const float value=item.first->load();if(value!=item.second){item.second=value;changed=true;}}
        const bool locked=processor.mixLocked.load();changed|=locked!=lastMixLock;lastMixLock=locked;
        if(changed)repaint();
        for(int i=0;i<4;++i){
            const float next=std::max(processor.meters[i].load(),meterValues[i]*.8f);
            if(std::abs(next-meterValues[i])>1.e-5f){meterValues[i]=next;repaint(i<2?juce::Rectangle<int>(74,32,52,154):juce::Rectangle<int>(1445,32,53,154));}
        }
    }
public:
    explicit LiveSurface(Processor& p):processor(p){
        setInterceptsMouseClicks(false,true);const auto b=skin::blue(),r=skin::red();
        for(const auto& item:p.values)observed.emplace_back(item.second,item.second->load());
        static const int thumbY[]={690,655,643,757,720,630,710,635};
        for(int i=0;i<8;++i){const int x=skin::stripX(i),cx=x+72;auto id=[&](const char* f){return Processor::id(i,f);};const auto colour=i<4?b:r;
            slider(common,id("level"),SkinSlider::fader,{cx-22,578,44,342},colour,{cx-18,thumbY[i],36,64},float(cx-64));
            slider(common,id("hp"),SkinSlider::knob,{x+22,461,44,44},colour);
            slider(common,id("lp"),SkinSlider::knob,{x+84,461,44,44},r);
            button(buttons,id("solo"),{x+34,537,37,28},{208,537,37,28},colour);
            button(buttons,id("mute"),{x+81,537,37,28},{255,537,37,28},colour);
            button(buttons,id("mute"),{32+(i/4)*57,275+(i%4)*43,51,36},{32+(i/4)*57,275+(i%4)*43,51,36},colour);
            if(i<4){
                const int routeX[]={190,337,485,632};
                for(int send=0;send<4;++send)slider(common,id(("send"+juce::String(send)).toRawUTF8()),SkinSlider::route,{routeX[i]+send*33,288,25,26},b,{},0,juce::String::charToString(juce::juce_wchar('A'+send)));
                button(buttons,id("direct"),{routeX[i],319,57,26},{190,319,57,26},b);
            }
        }
        slider(common,"input",SkinSlider::fader,{68,578,44,342},b,{72,717,36,64},116);
        slider(common,"output",SkinSlider::fader,{1454,578,44,342},r,{1458,717,36,64},1442);
        slider(common,"mix",SkinSlider::knob,{1461,461,43,44},b);
        for(int n=0;n<4;++n){
            delayTabs[n].setBounds(skin::stripX(n)+2,926,145,36);delayTabs[n].setComponentID("delayTab"+juce::String(n));delayTabs[n].onClick=[this,n]{selectDelay(n);};addAndMakeVisible(delayTabs[n]);
            reverbTabs[n].setBounds(skin::stripX(n+4)+2,926,145,36);reverbTabs[n].setComponentID("reverbTab"+juce::String(n));reverbTabs[n].onClick=[this,n]{selectReverb(n);};addAndMakeVisible(reverbTabs[n]);
        }
        delayPrevious.setBounds(476,52,31,38);delayPrevious.onClick=[this]{selectDelay((delayIndex+3)%4);};
        delayNext.setBounds(511,52,31,38);delayNext.onClick=[this]{selectDelay((delayIndex+1)%4);};
        reverbPrevious.setBounds(1104,52,31,38);reverbPrevious.onClick=[this]{selectReverb((reverbIndex+3)%4);};
        reverbNext.setBounds(1139,52,31,38);reverbNext.onClick=[this]{selectReverb((reverbIndex+1)%4);};
        for(auto* c:{&delayPrevious,&delayNext,&reverbPrevious,&reverbNext})addAndMakeVisible(*c);
        lock.setBounds(1510,451,32,33);lock.setComponentID("mixLock");lock.setTooltip("Bloquear MIX ao trocar preset");lock.onClick=[this]{processor.mixLocked.store(!processor.mixLocked.load());repaint();};addAndMakeVisible(lock);
        selectDelay(1);selectReverb(3);startTimerHz(20);
    }
    ~LiveSurface() override {stopTimer();}
    void showPreset(int index){
        const auto& s=epic::factoryPresets()[size_t(juce::jlimit(0,epic::factoryPresetCount-1,index))].settings;int d=0,r=4;
        for(int i=1;i<4;++i)if(!s.strips[i].mute&&(s.strips[d].mute||s.strips[i].level>s.strips[d].level))d=i;
        for(int i=5;i<8;++i)if(!s.strips[i].mute&&(s.strips[r].mute||s.strips[i].level>s.strips[r].level))r=i;
        selectDelay(d);selectReverb(r-4);
    }
    void paint(juce::Graphics& g) override {
        g.drawImage(image,getLocalBounds().toFloat());const auto b=skin::blue(),r=skin::red();
        const auto clip=g.getClipBounds();
        const bool metersOnly=juce::Rectangle<int>(74,32,52,154).contains(clip)||juce::Rectangle<int>(1445,32,53,154).contains(clip);
        if(!metersOnly){
        const char* delayNames[]={"TAPE","THROW","SLAP","CROWD"};const char* reverbNames[]={"PLATE","ROOM","HALL","SPACE"};
        if(delayIndex!=1){skin::display(g,{269,54,178,43},delayNames[delayIndex],b);skin::display(g,{207,60,23,35},juce::String(delayIndex+1),b);}
        if(reverbIndex!=3){skin::display(g,{885,54,190,43},reverbNames[reverbIndex],r);skin::display(g,{837,60,23,35},juce::String(3-reverbIndex),r);}
        for(int i=0;i<8;++i){const int x=skin::stripX(i);skin::display(g,{x+24,411,100,28},juce::String(processor.value(Processor::id(i,"level")),1),i<4?b:r,false);}
        skin::display(g,{1437,410,98,31},juce::String(processor.value("mix"),1),b,false);
        skin::display(g,{585,90,70,33},juce::String(processor.value(Processor::id(delayIndex,"mod")),1),b);
        skin::display(g,{1210,90,70,33},juce::String(processor.value(Processor::id(reverbIndex+4,"mod")),1),r);
        skin::display(g,{1158,194,96,33},juce::String(processor.value(Processor::id(reverbIndex+4,"decay")),2),r);
        for(int i=0;i<4;++i){const int routeX[]={190,337,485,632};skin::display(g,{routeX[i]+58,322,63,21},processor.value(Processor::id(i,"direct"))>.5f?"0.0":"--",b);}
        if(delayIndex!=1){g.setColour(juce::Colour(0xff081820));g.fillRect(266,112,145,24);}
        if(delayIndex>=2){
            skin::display(g,{483,145,100,32},delayIndex==2?"VSO":"WIDTH",b);
            g.setColour(juce::Colour(0xff081820));g.fillRect(490,184,259,64);g.fillRect(385,250,219,25);
        }
        if(reverbIndex!=3){g.setColour(juce::Colour(0xff22100c));g.fillRect(870,314,451,32);}
        for(int i=0;i<8;++i){
            const int x=skin::stripX(i);const bool active=i<4?i==delayIndex:i-4==reverbIndex;
            const bool originalActive=i==1||i==7;
            if(active!=originalActive){g.setColour(active?(i<4?b:r).withAlpha(.45f):juce::Colours::black.withAlpha(.65f));g.fillRect(x+2,926,145,34);}
        }
        }
        for(int side=0;side<2;++side)for(int ch=0;ch<2;++ch)for(int led=0;led<7;++led){
            const float db=juce::Decibels::gainToDecibels(meterValues[size_t(side*2+ch)],-60.f);const bool on=db>=-36+led*6;
            g.setColour(on?(led>4?juce::Colours::orangered:juce::Colours::limegreen):juce::Colour(0xff242620));
            g.fillEllipse(side==0?78.f+ch*28.5f:1449.f+ch*30.f,170.f-led*22.3f,13.f,14.f);
        }
        if(processor.mixLocked.load()){g.setColour(b.withAlpha(.6f));g.drawRoundedRectangle(1510,451,32,33,2,2);}
    }
};
class Editor final : public juce::AudioProcessorEditor,private juce::Timer {
    Processor& processor;LiveSurface surface;SkinPresets presets;juce::TooltipWindow tooltips {this,650};int displayedProgram=-1;
    void compatibleRenderer(){
       #if JUCE_WINDOWS
        if(auto* peer=getPeer()){
            const int software=peer->getAvailableRenderingEngines().indexOf("Software Renderer");
            if(software>=0&&peer->getCurrentRenderingEngine()!=software)peer->setCurrentRenderingEngine(software);
        }
       #endif
    }
    void timerCallback() override {
        const int n=processor.getCurrentProgram();presets.setSelectedId(n+1,juce::dontSendNotification);
        presets.setTooltip("PRESETS: "+processor.getProgramName(n)+" - clique no logotipo");
        if(n!=displayedProgram){surface.showPreset(n);displayedProgram=n;}
    }
public:
    explicit Editor(Processor& p):AudioProcessorEditor(p),processor(p),surface(p){
        addAndMakeVisible(surface);surface.setBounds(0,0,1567,1004);
        // Transparent combo occupies the existing logo, adding no toolbar or decoration.
        surface.addAndMakeVisible(presets);presets.setBounds(1427,245,119,79);presets.setComponentID("factoryPresets");
        for(int i=0;i<p.getNumPrograms();++i)presets.addItem(p.getProgramName(i),i+1);
        presets.onChange=[this]{processor.setCurrentProgram(presets.getSelectedId()-1);displayedProgram=-1;timerCallback();};
        timerCallback();setSize(1100,705);setResizable(false,false);startTimerHz(10);
    }
    ~Editor() override {stopTimer();}
    void parentHierarchyChanged() override {compatibleRenderer();}
    void visibilityChanged() override {if(isShowing())compatibleRenderer();}
    void paint(juce::Graphics& g) override {g.fillAll(juce::Colours::black);}
    void resized() override {
        const float scale=std::min(float(getWidth())/1567.f,float(getHeight())/1004.f);
        surface.setTransform(juce::AffineTransform::scale(scale).translated((getWidth()-1567*scale)*.5f,(getHeight()-1004*scale)*.5f));
    }
};
