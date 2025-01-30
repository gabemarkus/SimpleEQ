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

//==============================================================================
/**
*/
class SimpleEQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor&);
    ~SimpleEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleEQAudioProcessor& audioProcessor;
    
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessorEditor)
};
