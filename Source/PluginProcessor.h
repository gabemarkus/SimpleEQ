/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class SimpleEQAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SimpleEQAudioProcessor();
    ~SimpleEQAudioProcessor() override;

    //==============================================================================
    
    //ONE OF THE MOST IMPORTANT FUNCTIONS
    //CALLED BY THE HOST WHEN ITS ABOUT TO START PLAYBACK
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif
    
    //SECOND MOST IMPORTANT FUNCTION
    //CALLED WHEN PLAYBACK BEGINS IN THE HOST
    //THE HOST SENDS AUDIO BUFFER AKA AUDIO SAMPLES AND WE DO THE WORK ON THEM IN PROCESSBLOCK
    //THIS CANNOT BE INTERRUPTED - DO NOT ADD LATENCY AS THIS CAN CAUSE POPS AND CLICKS
    //YOU MUST FIGURE OUT HOW TO GET THE WORK DONE IN THE TIME YOU ARE GIVEN
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    
    //OUR VARIABLES
    
    //THIS SETS UP THE AUDIO PROCESSOR'S VALUE TREE
    //AN AUDIO PROCESSOR IS THE BASE CLASS FOR AUDIO PROCESSING CLASSES / PLUGINS
    //AN AUDIOPROCESSORVALUETREE MANAGES AN AUDIO PROCESSOR'S ENTIRE STATE
    //IT PROVIDES CHILD CLASSES FOR CONNECTING PARAMETERS TO GUI CONTROLS LIKE SLIDERS
    
    //function for creating parameter layout
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    //declaring audio processor value tree state
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, "Parameters", createParameterLayout()};
    
    private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
