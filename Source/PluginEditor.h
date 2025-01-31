/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

//THIS IS WHERE WE SET UP THE PLUGIN'S GUI AND VISUALS

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//creating a struct for creating knobs because they will all be the same
struct CustomRotarySlider : juce::Slider
{
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                                        juce::Slider::TextEntryBoxPosition::NoTextBox)
    {
        
    }
};

struct ResponseCurveComponent: juce::Component(),
juce::AudioProcessorParameter::Listener,
juce::Timer
{
    
}

//==============================================================================
/**
*/
class SimpleEQAudioProcessorEditor  : public juce::AudioProcessorEditor,
juce::AudioProcessorParameter::Listener,
juce::Timer

{
public:
    SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor&);
    ~SimpleEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    //we are overriding callbacks to check when the parameter value is changed
    //that way we can redraw the curve every time a parameter changes rather than every frame
    //because that would be very expensive
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {};
    //we need a timer to determine how often to check that the parameters change
    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleEQAudioProcessor& audioProcessor;
    
    juce::Atomic<bool> parametersChanged {false};
    
    CustomRotarySlider peakFreqSlider,
    peakGainSlider,
    peakQualitySlider,
    lowCutFreqSlider,
    highCutFreqSlider,
    lowCutSlopeSlider,
    highCutSlopeSlider;
    
    //now we attach our parameters to the knobs
    //the function to do is is very long so we create a typename alias to make it more readable
    
    using apvts = juce::AudioProcessorValueTreeState;
    using Attachment = apvts::SliderAttachment;
    
    //now we create the attachments
    Attachment peakFreqSliderAttachment,
    peakGainSliderAttachment,
    peakQualitySliderAttachment,
    lowCutFreqSliderAttachment,
    lowCutSlopeSliderAttachment,
    highCutFreqSliderAttachment,
    highCutSlopeSliderAttachment;
    
    
    //making vector to iterate through knobs
    std::vector<juce::Component*> GetComps();
    
    MonoChain monochain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessorEditor)
};
