/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//enum for hi/low pass slope options
enum Slope{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

//struct for all of our parameters
struct ChainSettings
{
    float peakFreq {0}, peakGainInDb{0}, peakQuality {.1f};
    float lowCutFreq{0}, highCutFreq{0};
    int lowCutSlope{Slope::Slope_12}, highCutSlope {Slope::Slope_12};
};
//now declaring a function to get these values, implemented in .cpp
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

//each filter type in IIR filter class has a response of 12db, so if we want a 48db slope we need 4 filters
//so we set up a chain and process context which will run through each element of the chain automatically
using Filter = juce::dsp::IIR::Filter<float>;
using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

//now we link together our cutfilters and filter to create the entire mono processing chain : Lowpass filter, peak EQ, Hipass filter

//most of the dsp modules process in mono so we will have a lot of duplicates
//so we will create some type aliases to eliminate some namespace and type definitions
using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

//we create an enum to represent each link's position in the chain to make it easier
enum ChainPositions{
    LowCut,
    Peak,
    HighCut
};

//creating helper function to update filter coefficients
using Coefficients = Filter::CoefficientsPtr;
void UpdateCoefficients(Coefficients& old, const Coefficients& replacement);

Coefficients MakePeakFilter(const ChainSettings& chainSettings, double sampleRate);

//creating helper function + template to simplify code in slope switch case
template<int Index, typename ChainType, typename CoefficientType>
void UpdateChain(ChainType& chain, const CoefficientType& cutCoefficients)
{
    UpdateCoefficients(chain.template get<Index>().coefficients, cutCoefficients[Index]);
    chain.template setBypassed<Index>(false);
}

//creating helper function to update hi/locut filter
//templates allow you to pass any type of variable and it will interpret it the same
template<typename ChainType, typename CoefficientType>
void UpdateCutFilter(ChainType& cutChain, const CoefficientType& cutCoefficients, const Slope& lowCutSlope)
{
    //we need to bypass all the links in the chain first
    cutChain.template setBypassed<0>(true);
    cutChain.template setBypassed<1>(true);
    cutChain.template setBypassed<2>(true);
    cutChain.template setBypassed<3>(true);
    //now we can switch between slopes with a switch case
    //note that we are not breaking between switch cases and that we are going in reverse order (48->12)
    //this is so that if the slope is 48, all the other cases also occur - all the filters will be activated
    //and if the slope is 12, only the 12 case will be used
    switch (lowCutSlope)
    {
        case Slope_48:
        {
            UpdateChain<3>(cutChain, cutCoefficients);
        }
        case Slope_36:
        {
            UpdateChain<2>(cutChain, cutCoefficients);
        }
        case Slope_24:
        {
            UpdateChain<1>(cutChain, cutCoefficients);
        }
        case Slope_12:
        {
            UpdateChain<0>(cutChain, cutCoefficients);
        }
    }
}

inline auto MakeLowCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    //now we create our lowcut filter coefficients
    //last argument = order aka how many 12db filters it will create
    //for some reason it creates one filter for every 2 orders
    //since we want 12/24/36/48db slopes we need max 8 orders
    //so we pass 2*(lowcutslope+1) to order (0->2, 1->4, 2->6, 3->8)
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, sampleRate, 2*(chainSettings.lowCutSlope + 1));
}


inline auto MakeHighCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq, sampleRate, 2*(chainSettings.highCutSlope + 1));
}

//
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

    
    //declare left and right chains
    MonoChain leftChain, rightChain;
    //now we need to prepare our chain to play so we go to prepareToPlay in .cpp
    
    //refactoring
    
    void UpdatePeakFilter(const ChainSettings& chainSettings);
    
    void UpdateAllFilters();
    void UpdateLowCutFilters(const ChainSettings& chainSettings);
    void UpdateHighCutFilters(const ChainSettings& chainSettings);
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
