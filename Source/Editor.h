#pragma once
#include <cstdlib>
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
        case 'Q':return bits({0,1,2,3,4,5,13});case 'L':return bits({3,4,5});case 'M':return bits({1,2,4,5,8,10});case 'N':return bits({1,2,4,5,8,13});case 'P':return bits({0,1,4,5,6,7});
        case 'R':return bits({0,1,4,5,6,7,13});case 'T':return bits({0,9,12});case 'U':return bits({1,2,3,4,5});case 'V':return bits({4,5,10,11});
        case 'X':return bits({8,10,11,13});case 'Z':return bits({0,1,6,7,4,3});case 'K':return bits({4,5,6,10,13});
        case 'W':return bits({1,2,4,5,11,13});case 'Y':return bits({8,10,12});case '-':return bits({6,7});case '/':return bits({10,11});default:return 0;
        }
    };
    text=text.toUpperCase();float width=0;for(auto c:text)width+=c=='.'?6.f:c==':'?10.f:24.f;width=std::max(1.f,width);
    const float scale=std::min(area.getWidth()/width,area.getHeight()/35.f);
    const float start=area.getX()+(area.getWidth()-width*scale)*.5f, top=area.getCentreY()-17.f*scale;
    float advance=0;for(auto c:text){const float x=start+advance*scale;advance+=c=='.'?6.f:c==':'?10.f:24.f;auto mask=glyph(c);
        if(c!='.'&&c!=':')for(int n=0;n<14;++n){g.setColour(colour.withAlpha((mask&(1u<<n))?.95f:.035f));const auto* l=lines[n];g.drawLine(x+l[0]*scale,top+l[1]*scale,x+l[2]*scale,top+l[3]*scale,std::max(1.5f,2.4f*scale));}
        if(c=='.'){g.setColour(colour);g.fillEllipse(x+1*scale,top+30*scale,3*scale,3*scale);}
        if(c==':'){g.setColour(colour);g.fillEllipse(x+3*scale,top+10*scale,3*scale,3*scale);g.fillEllipse(x+3*scale,top+22*scale,3*scale,3*scale);}
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
    Processor& processor;Kind kind;juce::Rectangle<int> source,thumb;juce::Image image=skin::original();juce::Colour colour;
    juce::String routeLetter,unit;float emptyX=0;juce::Point<int> down;double downValue=0;
    juce::RangedAudioParameter* parameter=nullptr;
    juce::TextEditor entry;bool editing=false,numberDragging=false;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    void activateForUser(double value){
        const auto id=getComponentID(),prefix=id.upToLastOccurrenceOf("_",false,false);
        auto toggle=[&](const juce::String& key,float v){if(auto* par=processor.state.getParameter(key))if(par->getValue()!=v){par->beginChangeGesture();par->setValueNotifyingHost(v);par->endChangeGesture();}};
        if(id.endsWith("_time")||id.endsWith("_time2")){
            toggle(prefix+(id.endsWith("_time2")?"_sync2":"_sync"),0);
            if(prefix=="s2")toggle("s2_vso",0);
        }
        if(id.endsWith("_beats")||id.endsWith("_beats2"))toggle(prefix+(id.endsWith("_beats2")?"_sync2":"_sync"),1);
        if(id=="s2_vso"&&value>0)toggle("s2_sync",0);
    }
    double increment() const {
        const auto id=getComponentID();
        if(id.endsWith("_feedback")||id.endsWith("_feedback2"))return .1;
        if(id.endsWith("_decay"))return .1;
        if(id.endsWith("_shape")||id.endsWith("_lowRatio")||id.endsWith("_highRatio"))return .01;
        if(id.endsWith("_offset"))return .1;
        return 1;
    }
    juce::String valueText() const {
        const auto id=getComponentID();
        if(id.endsWith("_beats")||id.endsWith("_beats2")){
            const double beat=getValue();
            for(int denominator:{1,2,4,8,16,32})for(int style=0;style<3;++style){
                const double candidate=4.0/denominator*(style==1?1.5:style==2?2.0/3.0:1.0);
                if(std::abs(candidate-beat)<.0001)return "1/"+juce::String(denominator)+(style==1?"D":style==2?"T":"");
            }
            return juce::String(beat,4).trimCharactersAtEnd("0").trimCharactersAtEnd(".");
        }
        if(id.endsWith("_vso")){const char* ips[]={"OFF","7.5","15","30"};return ips[juce::jlimit(0,3,int(getValue()))];}
        const int decimals=id.endsWith("_shape")||id.endsWith("Ratio")?2:getMaximum()>100?1:1;
        return juce::String(getValue(),decimals);
    }
    void openEntry(){
        if(kind!=number)return;
        editing=true;entry.setText(valueText(),false);entry.setVisible(true);entry.toFront(false);entry.grabKeyboardFocus();entry.selectAll();
    }
    void finishEntry(bool apply){
        if(!editing)return;editing=false;
        if(apply){
            auto text=entry.getText().trim().toUpperCase().replaceCharacter(',','.');double value=0;bool valid=true;
            const auto id=getComponentID();
            if(unit.isNotEmpty()&&text.endsWith(unit.toUpperCase()))text=text.dropLastCharacters(unit.length()).trim();
            if(id.endsWith("Ratio")&&text.startsWithChar('X'))text=text.substring(1).trim();
            if((id.endsWith("_beats")||id.endsWith("_beats2"))&&text.containsChar('/')){
                auto numeratorText=text.upToFirstOccurrenceOf("/",false,false);auto denominatorText=text.fromFirstOccurrenceOf("/",false,false);
                if(denominatorText.endsWithChar('D')||denominatorText.endsWithChar('T'))denominatorText=denominatorText.dropLastCharacters(1);
                char* endA=nullptr;char* endB=nullptr;const auto a=numeratorText.toRawUTF8(),b=denominatorText.toRawUTF8();
                const double numerator=std::strtod(a,&endA),denominator=std::strtod(b,&endB);
                valid=endA!=a&&*endA=='\0'&&endB!=b&&*endB=='\0'&&numerator>0&&denominator>0&&std::isfinite(numerator)&&std::isfinite(denominator);
                value=valid?4*numerator/denominator:0;
                if(text.endsWithChar('D'))value*=1.5;if(text.endsWithChar('T'))value*=2.0/3.0;
            }else if(id.endsWith("_vso")&&text=="OFF")value=0;
            else{
                auto bytes=text.toRawUTF8();char* end=nullptr;value=std::strtod(bytes,&end);
                valid=end!=bytes&&*end=='\0'&&std::isfinite(value);
                if(id.endsWith("_vso")){if(value==7.5)value=1;else if(value==15)value=2;else if(value==30)value=3;}
            }
            getProperties().set("lastValueEntryValid",valid);
            if(valid){parameter->beginChangeGesture();activateForUser(value);setValue(juce::jlimit(getMinimum(),getMaximum(),value),juce::sendNotificationSync);parameter->endChangeGesture();}
        }
        entry.setVisible(false);repaint();
    }
public:
    SkinSlider(Processor& p,const juce::String& id,Kind k,juce::Rectangle<int> rect,juce::Colour c,juce::Rectangle<int> sprite={},float cleanX=0,juce::String letter={})
      :processor(p),kind(k),source(rect),thumb(sprite),colour(c),routeLetter(letter),emptyX(cleanX),parameter(p.state.getParameter(id)){
        setComponentID(id);setBounds(rect);setName(parameter->getName(120));setTooltip(getName()+" - clique para digitar, arraste ou use a roda");
        setSliderStyle(k==fader?LinearVertical:k==knob?RotaryHorizontalVerticalDrag:LinearHorizontal);setTextBoxStyle(NoTextBox,false,0,0);
        setMouseDragSensitivity(300);setScrollWheelEnabled(true);setPopupDisplayEnabled(k!=number,k!=number,nullptr);
        addChildComponent(entry);entry.setComponentID(id+"_edit");entry.setMultiLine(false);entry.setJustification(juce::Justification::centred);
        entry.setColour(juce::TextEditor::textColourId,c);entry.setColour(juce::TextEditor::backgroundColourId,juce::Colour(0xff071017));entry.setColour(juce::TextEditor::outlineColourId,c);
        entry.onReturnKey=[this]{finishEntry(true);};entry.onEscapeKey=[this]{finishEntry(false);};entry.onFocusLost=[this]{finishEntry(isShowing());};
        attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,id,*this);
        setDoubleClickReturnValue(true,parameter->convertFrom0to1(parameter->getDefaultValue()));
        onValueChange=[this]{if(editing)entry.setText(valueText(),false);};resized();
    }
    ~SkinSlider() override {editing=false;entry.onFocusLost=nullptr;entry.onReturnKey=nullptr;entry.onEscapeKey=nullptr;onValueChange=nullptr;if(numberDragging)parameter->endChangeGesture();}
    void setUnit(juce::String text){unit=std::move(text);}
    void step(int direction){
        if(editing)finishEntry(true);parameter->beginChangeGesture();
        double value=getValue()+direction*increment();
        const auto id=getComponentID();
        if(id.endsWith("_beats")||id.endsWith("_beats2")){
            const double divisions[]={.125,1.0/6,.1875,.25,1.0/3,.375,.5,2.0/3,.75,1,1.5,2,3,4};
            value=direction>0?getMaximum():getMinimum();
            if(direction>0){for(double d:divisions)if(d>getValue()+.00001){value=d;break;}}
            else{for(double d:divisions)if(d<getValue()-.00001)value=d;}
        }
        activateForUser(value);setValue(juce::jlimit(getMinimum(),getMaximum(),value),juce::sendNotificationSync);parameter->endChangeGesture();
    }
    double valueToProportionOfLength(double value) override {
        if(kind!=fader)return juce::Slider::valueToProportionOfLength(value);
        if(getComponentID()=="input"||getComponentID()=="output")return juce::jlimit(0.0,1.0,(value+12.0)/24.0);
        const double y=value>=-40?655.0-value*4.65:841.0+(-40-value)*2.25;return juce::jlimit(0.0,1.0,(886.0-y)/276.0);
    }
    double proportionOfLengthToValue(double proportion) override {
        if(kind!=fader)return juce::Slider::proportionOfLengthToValue(proportion);
        if(getComponentID()=="input"||getComponentID()=="output")return proportion*24.0-12.0;
        const double y=886.0-proportion*276.0;return juce::jlimit(getMinimum(),getMaximum(),y<=841?(655.0-y)/4.65:-40.0-(y-841.0)/2.25);
    }
    void mouseDown(const juce::MouseEvent& e) override {down=e.getPosition();downValue=getValue();if(kind!=number)juce::Slider::mouseDown(e);}
    void mouseDrag(const juce::MouseEvent& e) override {
        if(kind==number){
            if(!numberDragging&&e.getPosition().getDistanceFrom(down)>3){numberDragging=true;parameter->beginChangeGesture();}
            if(numberDragging){const double value=downValue+(down.y-e.position.y+e.position.x-down.x)*increment();activateForUser(value);setValue(value,juce::sendNotificationSync);}
        }else if(kind==fader)setValue(proportionOfLengthToValue(juce::jlimit(0.0,1.0,1.0-(e.position.y-32.0)/276.0)),juce::sendNotificationSync);
        else juce::Slider::mouseDrag(e);
    }
    void mouseUp(const juce::MouseEvent& e) override {
        if(kind==number){if(numberDragging){numberDragging=false;parameter->endChangeGesture();}else openEntry();return;}
        if(kind==route&&e.getPosition().getDistanceFrom(down)<3)setValue(downValue>0?0:100,juce::sendNotificationSync);
        juce::Slider::mouseUp(e);
    }
    void mouseDoubleClick(const juce::MouseEvent& e) override {if(kind==number)openEntry();else juce::Slider::mouseDoubleClick(e);}
    void mouseWheelMove(const juce::MouseEvent& e,const juce::MouseWheelDetails& wheel) override {
        if(kind==number){const float delta=wheel.deltaY!=0?wheel.deltaY:wheel.deltaX;if(delta!=0)step(delta>0?1:-1);}
        else juce::Slider::mouseWheelMove(e,wheel);
    }
    void resized() override {entry.setBounds(getLocalBounds());entry.setFont(juce::Font(float(juce::jlimit(12,22,getHeight()-6))));}
    void paint(juce::Graphics& g) override {
        auto area=getLocalBounds();
        if(kind==knob){
            const auto sprite=image.getClippedImage(source);
            juce::Path outer;outer.addEllipse(area.toFloat().reduced(1));g.reduceClipRegion(outer);g.drawImageAt(sprite,0,0);
            const float cx=getWidth()*.5f,cy=getHeight()*.5f,radius=getWidth()>60?20.f:19.f;
            juce::Path circle;circle.addEllipse(cx-radius,cy-radius,2*radius,2*radius);g.reduceClipRegion(circle);
            const float baseline=getComponentID()=="mix"?.301f:0.f;
            const float angle=(float(valueToProportionOfLength(getValue()))-baseline)*4.712389f;
            g.drawImageTransformed(sprite,juce::AffineTransform::rotation(angle,cx,cy));
        }else if(kind==fader){
            g.drawImage(image,0,0,getWidth(),getHeight(),int(emptyX),source.getY(),getComponentID()=="output"?1:getWidth(),source.getHeight());const int cx=getWidth()/2;
            g.setColour(juce::Colour(0xff060809));g.fillRoundedRectangle(float(cx-4),32,8.f,276.f,3.f);
            const float t=float(1-valueToProportionOfLength(getValue()));const int y=int(32+t*276)-thumb.getHeight()/2;
            g.drawImage(image,cx-thumb.getWidth()/2,y,thumb.getWidth(),thumb.getHeight(),thumb.getX(),thumb.getY(),thumb.getWidth(),thumb.getHeight());
        }else if(kind==route){
            g.setColour(getValue()>0?colour.withAlpha(.8f):juce::Colour(0xff10222b));g.fillRect(area.reduced(1));g.setColour(colour);g.drawRect(area.reduced(1));
            g.setColour(getValue()>0?juce::Colours::black:colour);g.setFont(17.f);g.drawText(routeLetter,area,juce::Justification::centred);
        }else{
            auto textArea=area;if(unit.isNotEmpty())textArea.removeFromRight(unit=="sec"?31:25);
            skin::display(g,area,"",colour,source.getY()<380);skin::digital(g,valueText(),textArea.toFloat().reduced(2),colour);
            if(unit.isNotEmpty()){g.setColour(colour);g.setFont(14.f);g.drawText(unit,area.removeFromRight(unit=="sec"?31:25),juce::Justification::centred);}
        }
    }
};
class ValueArrow final : public juce::TextButton {
    juce::Colour colour;bool up;
public:
    ValueArrow(SkinSlider& slider,bool increase,juce::Rectangle<int> rect,juce::Colour c):colour(c),up(increase){
        setBounds(rect);setComponentID(slider.getComponentID()+(up?"_up":"_down"));setTooltip(slider.getName()+(up?" +":" -"));
        juce::Component::SafePointer<SkinSlider> safe(&slider);onClick=[safe,increase]{if(safe)safe->step(increase?1:-1);};setRepeatSpeed(400,75);
    }
    void paintButton(juce::Graphics& g,bool over,bool pressed) override {
        g.setColour(colour.withAlpha(pressed?1.f:over?.95f:.7f));const float cx=getWidth()*.5f,cy=getHeight()*.5f;
        juce::Path p;p.startNewSubPath(cx-5,cy+(up?2:-2));p.lineTo(cx,cy+(up?-3:3));p.lineTo(cx+5,cy+(up?2:-2));g.strokePath(p,juce::PathStrokeType(2));
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
    SkinPresets(){for(auto* c:getChildren())c->setVisible(false);setName("200 presets de fabrica");}
    void paint(juce::Graphics& g) override {
        auto bounds=getLocalBounds();g.setColour(juce::Colour(0xffc5cbd0));g.setFont(14.f);g.drawText("PRESETS",bounds.removeFromTop(18),juce::Justification::centred);
        g.setColour(juce::Colour(0xff101c24));g.fillRoundedRectangle(bounds.toFloat().reduced(1),3);g.setColour(skin::blue().withAlpha(.7f));g.drawRoundedRectangle(bounds.toFloat().reduced(1),3,1);
        auto text=bounds.reduced(5,1);text.removeFromRight(12);g.setFont(12.f);g.drawFittedText(getText(),text,juce::Justification::centredLeft,1,.7f);
        juce::Path arrow;arrow.addTriangle(float(getWidth()-15),30,float(getWidth()-7),30,float(getWidth()-11),35);g.fillPath(arrow);
    }
    void resized() override {juce::ComboBox::resized();for(auto* c:getChildren())c->setVisible(false);}
};
class LiveSurface final : public juce::Component,private juce::Timer {
    Processor& processor;juce::Image image=skin::original();
    std::vector<std::unique_ptr<SkinSlider>> common,delayFields,reverbFields;
    std::vector<std::unique_ptr<SkinButton>> buttons,delayButtons;
    std::vector<std::unique_ptr<ValueArrow>> delayArrows,reverbArrows;
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
    SkinSlider& number(std::vector<std::unique_ptr<SkinSlider>>& fields,std::vector<std::unique_ptr<ValueArrow>>& arrows,const juce::String& id,juce::Rectangle<int> rect,juce::String unit,juce::Colour c,int arrowX=0){
        auto& value=slider(fields,id,SkinSlider::number,rect,c);value.setUnit(std::move(unit));
        if(arrowX){for(bool up:{true,false}){auto a=std::make_unique<ValueArrow>(value,up,juce::Rectangle<int>(arrowX,rect.getY()+(up?0:rect.getHeight()/2),18,rect.getHeight()/2),c);addAndMakeVisible(*a);arrows.push_back(std::move(a));}}
        return value;
    }
    void selectDelay(int i){
        delayIndex=i;delayArrows.clear();delayFields.clear();delayButtons.clear();const auto c=skin::blue();auto id=[&](const char* f){return Processor::id(i,f);};
        number(delayFields,delayArrows,id("time"),{336,143,105,36},"ms",c,441);
        number(delayFields,delayArrows,id("feedback"),{288,213,97,31},"%",c,385);
        number(delayFields,delayArrows,id("beats"),{313,184,112,28},"",c,442);
        button(delayButtons,id("sync"),{204,187,38,25},{204,187,38,25},c);
        slider(delayFields,id("mod"),SkinSlider::knob,{672,49,66,66},c);
        number(delayFields,delayArrows,id("mod"),{585,90,70,33},"",c);
        if(i<2){
            number(delayFields,delayArrows,id("time2"),{628,143,105,36},"ms",c,733);
            number(delayFields,delayArrows,id("feedback2"),{574,213,97,31},"%",c,671);
            number(delayFields,delayArrows,id("beats2"),{599,184,112,28},"",c,734);
            number(delayFields,delayArrows,id("offset"),{493,246,83,29},"%",c,577);
            button(delayButtons,id("sync2"),{490,187,38,25},{204,187,38,25},c);
        }
        if(i==1){button(delayButtons,id("triggerEnabled"),{339,115,38,18},{339,115,38,18},c);button(delayButtons,id("trigger"),{381,115,27,18},{381,115,27,18},c);}
        if(i==2)number(delayFields,delayArrows,id("vso"),{628,143,105,36},"IPS",c,733);
        if(i==3)number(delayFields,delayArrows,id("width"),{628,143,105,36},"%",c,733);
        repaint();
    }
    void selectReverb(int r){
        reverbIndex=r;reverbArrows.clear();reverbFields.clear();const auto c=skin::red();auto id=[&](const char* f){return Processor::id(r+4,f);};
        number(reverbFields,reverbArrows,id("predelay"),{932,193,95,36},"ms",c,1030);
        number(reverbFields,reverbArrows,id("decay"),{1174,259,98,39},"sec",c,1275);
        slider(reverbFields,id("mod"),SkinSlider::knob,{1294,49,66,66},c);
        number(reverbFields,reverbArrows,id("mod"),{1210,90,70,33},"",c);
        number(reverbFields,reverbArrows,id("lowRatio"),{1018,314,56,31},"",c,1077);
        number(reverbFields,reverbArrows,id("highRatio"),{1252,314,56,31},"",c,1311);
        if(r==3)number(reverbFields,reverbArrows,id("shape"),{1158,193,139,36},"",c,1303);
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
            slider(common,id("level"),SkinSlider::number,{x+24,411,100,28},colour);
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
        slider(common,"mix",SkinSlider::number,{1437,410,98,31},b);
        for(int n=0;n<4;++n){
            delayTabs[n].setBounds(skin::stripX(n)+2,926,145,36);delayTabs[n].setComponentID("delayTab"+juce::String(n));delayTabs[n].onClick=[this,n]{selectDelay(n);};addAndMakeVisible(delayTabs[n]);
            reverbTabs[n].setBounds(skin::stripX(n+4)+2,926,145,36);reverbTabs[n].setComponentID("reverbTab"+juce::String(n));reverbTabs[n].onClick=[this,n]{selectReverb(n);};addAndMakeVisible(reverbTabs[n]);
        }
        delayPrevious.setBounds(476,52,31,38);delayPrevious.onClick=[this]{selectDelay((delayIndex+3)%4);};
        delayNext.setBounds(511,52,31,38);delayNext.onClick=[this]{selectDelay((delayIndex+1)%4);};
        reverbPrevious.setBounds(1130,52,31,38);reverbPrevious.onClick=[this]{selectReverb((reverbIndex+3)%4);};
        reverbNext.setBounds(1165,52,31,38);reverbNext.onClick=[this]{selectReverb((reverbIndex+1)%4);};
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
    void paintLCD(juce::Graphics& g){
        const auto b=skin::blue(),r=skin::red();
        skin::display(g,{184,32,580,346},"",b);skin::display(g,{810,32,582,346},"",r);
        auto caption=[&](juce::String text,juce::Rectangle<int> rect,juce::Colour colour){skin::digital(g,text,rect.toFloat(),colour);};
        auto box=[&](juce::Rectangle<int> rect,juce::Colour colour){g.setColour(colour.withAlpha(.5f));g.drawRect(rect,1);};
        const char* delayNames[]={"TAPE","THROW","SLAP","CROWD"};const char* reverbNames[]={"PLATE","ROOM","HALL","SPACE"};
        g.setColour(b);g.fillRect(202,58,38,44);caption(juce::String(delayIndex+1),{206,61,29,36},juce::Colour(0xff082231));
        g.setColour(r);g.fillRect(835,58,38,44);caption(juce::String(3-reverbIndex),{839,61,29,36},juce::Colour(0xff36140d));
        caption(delayNames[delayIndex],{267,54,155,43},b);
        caption(reverbNames[reverbIndex],{885,54,180,43},r);
        caption("FX:",{434,62,35,22},b);caption("FX:",{1084,62,35,22},r);
        for(int i=0;i<4;++i){
            const int positions[]={476,511,1130,1165};const float cx=positions[i]+15.5f,cy=71;
            g.setColour(i<2?b:r);juce::Path arrow;
            arrow.startNewSubPath(cx+(i%2==0?5:-5),cy-9);arrow.lineTo(cx+(i%2==0?-4:4),cy);arrow.lineTo(cx+(i%2==0?5:-5),cy+9);
            g.strokePath(arrow,juce::PathStrokeType(4));
        }
        caption("MOD",{604,59,56,21},b);caption("MOD",{1230,59,56,21},r);
        caption("0",{664,113,14,14},b);caption("100",{720,113,31,14},b);caption("0",{1290,113,14,14},r);caption("100",{1347,113,31,14},r);
        if(delayIndex==1)caption("TRIGGER",{260,114,73,19},b);
        box({194,140,274,123},b);box({482,140,274,123},b);
        caption("TAP 1:",{204,147,116,30},b);caption("SYNC:",{245,187,65,24},b);caption("FDBK:",{203,216,79,26},b);
        if(delayIndex<2){caption("TAP 2:",{490,147,116,30},b);caption("SYNC:",{531,187,65,24},b);caption("FDBK:",{490,216,79,26},b);caption("OFFSET:",{392,248,95,24},b);}
        else caption(delayIndex==2?"VSO:":"WIDTH:",{490,147,126,30},b);
        caption("ROUTING",{410,350,140,24},b);
        const int routeX[]={190,337,485,632};
        for(int i=0;i<4;++i)skin::display(g,{routeX[i]+58,322,63,21},processor.value(Processor::id(i,"direct"))>.5f?"0.0":"--",b);
        box({817,140,278,102},r);caption("PRE DELAY:",{834,158,244,32},r);
        if(reverbIndex==3){box({1103,140,277,102},r);caption("DECAY:",{1140,158,210,32},r);}
        box({817,249,563,111},r);caption("REVERB TIME:",{879,260,269,37},r);
        caption("RT LOW: X",{893,317,121,26},r);caption("RT HIGH: X",{1105,317,144,26},r);
    }
    void paint(juce::Graphics& g) override {
        g.drawImage(image,getLocalBounds().toFloat());const auto b=skin::blue(),r=skin::red();
        const auto clip=g.getClipBounds();
        const bool metersOnly=juce::Rectangle<int>(74,32,52,154).expanded(2).contains(clip)||juce::Rectangle<int>(1445,32,53,154).expanded(2).contains(clip);
        if(!metersOnly){
            paintLCD(g);
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
    Processor& processor;LiveSurface surface;SkinPresets presets;SkinAction presetLogoShortcut;juce::TooltipWindow tooltips {this,650};int displayedProgram=-1;
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
        presets.setTooltip("200 PRESETS: "+processor.getProgramName(n)+" - selecione no menu");
        if(n!=displayedProgram){surface.showPreset(n);displayedProgram=n;}
    }
public:
    explicit Editor(Processor& p):AudioProcessorEditor(p),processor(p),surface(p){
        addAndMakeVisible(surface);surface.setBounds(0,0,1567,1004);
        surface.addAndMakeVisible(presets);presets.setBounds(1427,328,119,48);presets.setComponentID("factoryPresets");
        surface.addAndMakeVisible(presetLogoShortcut);presetLogoShortcut.setBounds(1427,245,119,79);presetLogoShortcut.setComponentID("presetLogoShortcut");presetLogoShortcut.onClick=[this]{presets.showPopup();};
        for(int i=0;i<p.getNumPrograms();++i)presets.addItem(juce::String(i+1).paddedLeft('0',3)+" "+p.getProgramName(i),i+1);
        presets.onChange=[this]{processor.setCurrentProgram(presets.getSelectedId()-1);displayedProgram=-1;timerCallback();};
        timerCallback();setSize(1100,705);setResizable(false,false);startTimerHz(10);
    }
    ~Editor() override {stopTimer();presets.hidePopup();juce::PopupMenu::dismissAllActiveMenus();}
    void parentHierarchyChanged() override {compatibleRenderer();}
    void visibilityChanged() override {if(isShowing())compatibleRenderer();}
    void paint(juce::Graphics& g) override {g.fillAll(juce::Colours::black);}
    void resized() override {
        const float scale=std::min(float(getWidth())/1567.f,float(getHeight())/1004.f);
        surface.setTransform(juce::AffineTransform::scale(scale).translated((getWidth()-1567*scale)*.5f,(getHeight()-1004*scale)*.5f));
    }
};
