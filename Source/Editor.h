#pragma once
// The original PNG is the skin. Only existing control footprints are repainted.
namespace skin {
inline juce::Colour blue(){return juce::Colour(0xff8ddfff);}
inline juce::Colour red(){return juce::Colour(0xffff492a);}
inline juce::Image original(){return juce::ImageCache::getFromMemory(BinaryData::imagem_png,BinaryData::imagem_pngSize);}
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
    text=text.toUpperCase();const float width=std::max(1.f,float(text.length())*24.f);
    const float scale=std::min(area.getWidth()/width,area.getHeight()/35.f);
    const float start=area.getX()+(area.getWidth()-width*scale)*.5f, top=area.getCentreY()-17.f*scale;
    int index=0;for(auto c:text){const float x=start+index++*24*scale;auto mask=glyph(c);
        for(int n=0;n<14;++n){g.setColour(colour.withAlpha((mask&(1u<<n))?.95f:.035f));const auto* l=lines[n];g.drawLine(x+l[0]*scale,top+l[1]*scale,x+l[2]*scale,top+l[3]*scale,std::max(1.f,1.5f*scale));}
        if(c=='.'){g.setColour(colour);g.fillEllipse(x+5*scale,top+30*scale,3*scale,3*scale);}
    }
}
inline void display(juce::Graphics& g,juce::Rectangle<int> rect,const juce::String& value,juce::Colour c){
    g.setColour(c==red()?juce::Colour(0xff20100c):juce::Colour(0xff07151e));g.fillRect(rect);
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
    void mouseDown(const juce::MouseEvent& e) override {down=e.getPosition();downValue=getValue();juce::Slider::mouseDown(e);}
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
            g.drawImage(image,0,0,getWidth(),getHeight(),int(emptyX),source.getY(),getWidth(),source.getHeight());
            const int cx=getWidth()/2;
            g.setColour(juce::Colour(0xff060809));g.fillRoundedRectangle(float(cx-4),0,8.f,float(getHeight()),3.f);
            const float t=float(1-valueToProportionOfLength(getValue()));const int y=int(t*float(getHeight()-thumb.getHeight()));
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
            g.drawImage(image,0,0,getWidth(),getHeight(),sprite.getX()<80?32:89,275,51,36);
            if(getToggleState()){g.setColour(colour.withAlpha(.45f));g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(2),2);}
            g.setColour(getToggleState()?colour.withAlpha(.7f):juce::Colour(0xff17252d));g.fillRect(getWidth()/2-11,5,23,26);
            const int index=(sprite.getY()-275)/43;
            const auto label=sprite.getX()<80?juce::String(index+1):juce::String::charToString(juce::juce_wchar('A'+index));
            skin::digital(g,label,{float(getWidth()/2-9),6,19,24},getToggleState()?juce::Colours::black:colour.withAlpha(.7f));
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
    SkinSlider& slider(std::vector<std::unique_ptr<SkinSlider>>& list,const juce::String& id,SkinSlider::Kind kind,juce::Rectangle<int> rect,juce::Colour colour,juce::Rectangle<int> thumb={},float cleanX=0,juce::String letter={}){
        auto c=std::make_unique<SkinSlider>(processor,id,kind,rect,colour,thumb,cleanX,letter);auto& result=*c;addAndMakeVisible(*c);list.push_back(std::move(c));return result;
    }
    void button(std::vector<std::unique_ptr<SkinButton>>& list,const juce::String& id,juce::Rectangle<int> rect,juce::Rectangle<int> source,juce::Colour colour){
        auto c=std::make_unique<SkinButton>(processor,id,rect,source,colour);addAndMakeVisible(*c);list.push_back(std::move(c));
    }
    void selectDelay(int i){
        delayIndex=i;delayFields.clear();delayButtons.clear();const auto c=skin::blue();auto id=[&](const char* f){return Processor::id(i,f);};
        slider(delayFields,id("time"),SkinSlider::number,{319,145,104,34},c);
        slider(delayFields,id("feedback"),SkinSlider::number,{326,219,86,27},c);
        slider(delayFields,id("beats"),SkinSlider::number,{285,187,113,25},c);
        button(delayButtons,id("sync"),{204,187,38,25},{204,187,38,25},c);
        slider(delayFields,id("mod"),SkinSlider::knob,{672,49,66,66},c);
        if(i<2){
            slider(delayFields,id("time2"),SkinSlider::number,{610,145,104,34},c);
            slider(delayFields,id("feedback2"),SkinSlider::number,{615,219,86,27},c);
            slider(delayFields,id("beats2"),SkinSlider::number,{575,187,113,25},c);
            slider(delayFields,id("offset"),SkinSlider::number,{518,250,84,25},c);
            button(delayButtons,id("sync2"),{490,187,38,25},{204,187,38,25},c);
        }
        if(i==1){button(delayButtons,id("triggerEnabled"),{339,115,38,18},{339,115,38,18},c);button(delayButtons,id("trigger"),{381,115,27,18},{381,115,27,18},c);}
        if(i==2)slider(delayFields,id("vso"),SkinSlider::number,{610,145,104,34},c);
        if(i==3)slider(delayFields,id("width"),SkinSlider::number,{610,145,104,34},c);
        repaint();
    }
    void selectReverb(int r){
        reverbIndex=r;reverbFields.clear();const auto c=skin::red();auto id=[&](const char* f){return Processor::id(r+4,f);};
        slider(reverbFields,id("predelay"),SkinSlider::number,{919,191,137,38},c);
        slider(reverbFields,id("decay"),SkinSlider::number,{1168,267,98,40},c);
        slider(reverbFields,id("mod"),SkinSlider::knob,{1294,49,66,66},c);
        if(r==3){slider(reverbFields,id("lowRatio"),SkinSlider::number,{1003,318,69,26},c);slider(reverbFields,id("highRatio"),SkinSlider::number,{1240,318,69,26},c);}
        repaint();
    }
    void timerCallback() override {for(int i=0;i<4;++i)meterValues[i]=std::max(processor.meters[i].load(),meterValues[i]*.85f);repaint();}
public:
    explicit LiveSurface(Processor& p):processor(p){
        setInterceptsMouseClicks(false,true);const auto b=skin::blue(),r=skin::red();
        static const int thumbY[]={690,655,643,757,720,630,710,635};
        for(int i=0;i<8;++i){const int x=skin::stripX(i),cx=x+72;auto id=[&](const char* f){return Processor::id(i,f);};const auto colour=i<4?b:r;
            slider(common,id("level"),SkinSlider::fader,{cx-22,605,44,285},colour,{cx-18,thumbY[i],36,64},float(cx-64));
            slider(common,id("hp"),SkinSlider::knob,{x+22,461,44,44},colour);
            slider(common,id("lp"),SkinSlider::knob,{x+84,461,44,44},r);
            button(buttons,id("solo"),{x+34,537,37,28},{208,537,37,28},colour);
            button(buttons,id("mute"),{x+81,537,37,28},{255,537,37,28},colour);
            button(buttons,id("mute"),{32+(i/4)*57,275+(i%4)*43,51,36},{32+(i/4)*57,275+(i%4)*43,51,36},colour);
            if(i<4){
                for(int send=0;send<4;++send)slider(common,id(("send"+juce::String(send)).toRawUTF8()),SkinSlider::route,{194+i*147+send*33,288,26,25},b,{},0,juce::String::charToString(juce::juce_wchar('A'+send)));
                button(buttons,id("direct"),{193+i*147,319,56,25},{193,319,56,25},b);
            }
        }
        slider(common,"input",SkinSlider::fader,{68,605,44,285},b,{72,717,36,64},116);
        slider(common,"output",SkinSlider::fader,{1454,605,44,285},r,{1458,717,36,64},1404);
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
        selectDelay(1);selectReverb(3);startTimerHz(25);
    }
    void showPreset(int index){
        const auto& s=epic::factoryPresets()[size_t(juce::jlimit(0,99,index))].settings;int d=0,r=4;
        for(int i=1;i<4;++i)if(!s.strips[i].mute&&(s.strips[d].mute||s.strips[i].level>s.strips[d].level))d=i;
        for(int i=5;i<8;++i)if(!s.strips[i].mute&&(s.strips[r].mute||s.strips[i].level>s.strips[r].level))r=i;
        selectDelay(d);selectReverb(r-4);
    }
    void paint(juce::Graphics& g) override {
        g.drawImage(image,getLocalBounds().toFloat());const auto b=skin::blue(),r=skin::red();
        const char* delayNames[]={"TAPE","THROW","SLAP","CROWD"};const char* reverbNames[]={"PLATE","ROOM","HALL","SPACE"};
        if(delayIndex!=1){skin::display(g,{269,54,178,43},delayNames[delayIndex],b);skin::display(g,{207,60,23,35},juce::String(delayIndex+1),b);}
        if(reverbIndex!=3){skin::display(g,{885,54,190,43},reverbNames[reverbIndex],r);skin::display(g,{837,60,23,35},juce::String(3-reverbIndex),r);}
        for(int i=0;i<8;++i){const int x=skin::stripX(i);skin::display(g,{x+24,411,100,28},juce::String(processor.value(Processor::id(i,"level")),1),i<4?b:r);}
        skin::display(g,{1442,412,84,28},juce::String(processor.value("mix"),1),b);
        skin::display(g,{589,96,68,29},juce::String(processor.value(Processor::id(delayIndex,"mod")),1),b);
        skin::display(g,{1215,96,68,29},juce::String(processor.value(Processor::id(reverbIndex+4,"mod")),1),r);
        skin::display(g,{1214,191,100,38},juce::String(processor.value(Processor::id(reverbIndex+4,"decay")),2),r);
        for(int i=0;i<4;++i){skin::display(g,{252+i*147,323,54,18},processor.value(Processor::id(i,"direct"))>.5f?"0.0":"--",b);}
        if(delayIndex!=1){g.setColour(juce::Colour(0xff081820));g.fillRect(266,112,145,24);}
        if(delayIndex>=2){
            skin::display(g,{495,145,105,32},delayIndex==2?"VSO":"WIDTH",b);
            g.setColour(juce::Colour(0xff081820));g.fillRect(490,184,259,64);g.fillRect(385,250,219,25);
        }
        if(reverbIndex!=3){g.setColour(juce::Colour(0xff22100c));g.fillRect(870,314,451,32);}
        for(int i=0;i<8;++i){
            const int x=skin::stripX(i);g.drawImage(image,x+2,926,145,36,176,926,145,36);
            const bool active=i<4?i==delayIndex:i-4==reverbIndex;
            if(active){g.setColour((i<4?b:r).withAlpha(.45f));g.fillRect(x+2,926,145,34);}
            g.setColour(juce::Colour(0xff252b2e));g.fillRect(x+60,928,26,30);
            skin::digital(g,i<4?juce::String(i+1):juce::String::charToString(juce::juce_wchar('A'+i-4)),{float(x+60),929,26,28},juce::Colours::lightgrey);
        }
        for(int side=0;side<2;++side)for(int ch=0;ch<2;++ch)for(int led=0;led<7;++led){
            const float db=juce::Decibels::gainToDecibels(meterValues[size_t(side*2+ch)],-60.f);const bool on=db>=-36+led*6;
            g.setColour(on?(led>4?juce::Colours::orangered:juce::Colours::limegreen):juce::Colour(0xff242620));
            g.fillEllipse(float((side==0?76:1448)+ch*29),float(171-led*22),11.f,11.f);
        }
        if(processor.mixLocked.load()){g.setColour(b.withAlpha(.6f));g.drawRoundedRectangle(1510,451,32,33,2,2);}
    }
};
class Editor final : public juce::AudioProcessorEditor,private juce::Timer {
    Processor& processor;LiveSurface surface;SkinPresets presets;juce::TooltipWindow tooltips {this,650};int displayedProgram=-1;
    void timerCallback() override {
        const int n=processor.getCurrentProgram();presets.setSelectedId(n+1,juce::dontSendNotification);
        presets.setTooltip("PRESETS: "+processor.getProgramName(n)+" - clique no logotipo");
        if(n!=displayedProgram){surface.showPreset(n);displayedProgram=n;}
    }
public:
    explicit Editor(Processor& p):AudioProcessorEditor(p),processor(p),surface(p){
        addAndMakeVisible(surface);surface.setBounds(0,0,1568,1008);
        // Transparent combo occupies the existing logo, adding no toolbar or decoration.
        surface.addAndMakeVisible(presets);presets.setBounds(1427,245,119,79);presets.setComponentID("factoryPresets");
        for(int i=0;i<100;++i)presets.addItem(p.getProgramName(i),i+1);
        presets.onChange=[this]{processor.setCurrentProgram(presets.getSelectedId()-1);displayedProgram=-1;timerCallback();};
        timerCallback();setSize(1100,707);setResizable(true,true);setResizeLimits(840,540,1882,1210);
        if(auto* constrainer=getConstrainer())constrainer->setFixedAspectRatio(1568.0/1008.0);startTimerHz(10);
    }
    void paint(juce::Graphics& g) override {g.fillAll(juce::Colours::black);}
    void resized() override {
        const float scale=std::min(float(getWidth())/1568.f,float(getHeight())/1008.f);
        surface.setTransform(juce::AffineTransform::scale(scale).translated((getWidth()-1568*scale)*.5f,(getHeight()-1008*scale)*.5f));
    }
};
