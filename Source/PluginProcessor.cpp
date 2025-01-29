/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessor::SimpleEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

SimpleEQAudioProcessor::~SimpleEQAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    //we need to create a ProcessSpec to tell our chain how it will process audio
    
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock; //how many samples per block/ how many it will process at once
    spec.numChannels = 1; //how many channels to process
    spec.sampleRate = sampleRate; //sample rate
    
    //now we pass this spec to each chain
    leftChain.prepare(spec);
    rightChain.prepare(spec);
    
    //creating peak filter coefficients
    auto chainSettings = getChainSettings(apvts);
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, chainSettings.peakFreq, chainSettings.peakQuality, juce::Decibels::decibelsToGain(chainSettings.peakGainInDb));
    
    *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    
    //at this point the peak filter could make changes to the audio but as we have not yet set up our sliders nothing will change
    //we do that in processblock
    
    //now we create our lowcut filter coefficient
    //last argument = order aka how many 12db filters it will create
    //for some reason it creates one filter for every 2 orders
    //since we want 12/24/36/48db slopes we need max 8 orders
    //so we pass 2*(lowcutslope+1) to order (0->2, 1->4, 2->6, 3->8)
    auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, sampleRate, 2*(chainSettings.lowCutSlope + 1));
    
    //set up lowcut filter
    //remember we need to split channels
    //left chain first
    auto &leftLowCut = leftChain.get<ChainPositions::LowCut>();
    //we need to bypass all the links in the chain first
    leftLowCut.setBypassed<0>(true);
    leftLowCut.setBypassed<1>(true);
    leftLowCut.setBypassed<2>(true);
    leftLowCut.setBypassed<3>(true);
    //now we can switch between slopes with a switch case
    switch (chainSettings.lowCutSlope)
    {
        case Slope_12:
            //assign coefficient to first filter
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            //stop bypassing first filter
            leftLowCut.setBypassed<0>(false);
            //etc
            break;
        case Slope_24:
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<0>(false);
            *leftLowCut.get<1>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<1>(false);
            break;
        case Slope_36:
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<0>(false);
            *leftLowCut.get<1>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<1>(false);
            *leftLowCut.get<2>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<2>(false);
            break;
        case Slope_48:
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<0>(false);
            *leftLowCut.get<1>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<1>(false);
            *leftLowCut.get<2>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<2>(false);
            *leftLowCut.get<3>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<3>(false);
            break;
        default:
            break;
    }
    
    //repeat for right chain
    
    auto &rightLowCut = rightChain.get<ChainPositions::LowCut>();
    //we need to bypass all the links in the chain first
    rightLowCut.setBypassed<0>(true);
    rightLowCut.setBypassed<1>(true);
    rightLowCut.setBypassed<2>(true);
    rightLowCut.setBypassed<3>(true);
    //now we can switch between slopes with a switch case
    switch (chainSettings.lowCutSlope)
    {
        case Slope_12:
            //assign coefficient to first filter
            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
            //stop bypassing first filter
            rightLowCut.setBypassed<0>(false);
            //etc
            break;
        case Slope_24:
            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<0>(false);
            *rightLowCut.get<1>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<1>(false);
            break;
        case Slope_36:
            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<0>(false);
            *rightLowCut.get<1>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<1>(false);
            *rightLowCut.get<2>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<2>(false);
            break;
        case Slope_48:
            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<0>(false);
            *rightLowCut.get<1>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<1>(false);
            *rightLowCut.get<2>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<2>(false);
            *rightLowCut.get<3>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<3>(false);
            break;
        default:
            break;
    }
    
}

void SimpleEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

//THIS IS WHERE WE GET THE BLOCK OF AUDIO DATA IN THE FORM OF A BUFFER AND MIDI MESSAGES
void SimpleEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    ///
    
    //setting up parameters BEFORE processing audio = very important (obviously)
    
    auto chainSettings = getChainSettings(apvts);
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), chainSettings.peakFreq, chainSettings.peakQuality, juce::Decibels::decibelsToGain(chainSettings.peakGainInDb));
    
    *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    
    auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, getSampleRate(), 2*(chainSettings.lowCutSlope + 1));
    
    //set up lowcut filter
    //remember we need to split channels
    //left chain first
    auto &leftLowCut = leftChain.get<ChainPositions::LowCut>();
    //we need to bypass all the links in the chain first
    leftLowCut.setBypassed<0>(true);
    leftLowCut.setBypassed<1>(true);
    leftLowCut.setBypassed<2>(true);
    leftLowCut.setBypassed<3>(true);
    //now we can switch between slopes with a switch case
    switch (chainSettings.lowCutSlope)
    {
        case Slope_12:
            //assign coefficient to first filter
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            //stop bypassing first filter
            leftLowCut.setBypassed<0>(false);
            //etc
            break;
        case Slope_24:
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<0>(false);
            *leftLowCut.get<1>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<1>(false);
            break;
        case Slope_36:
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<0>(false);
            *leftLowCut.get<1>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<1>(false);
            *leftLowCut.get<2>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<2>(false);
            break;
        case Slope_48:
            *leftLowCut.get<0>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<0>(false);
            *leftLowCut.get<1>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<1>(false);
            *leftLowCut.get<2>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<2>(false);
            *leftLowCut.get<3>().coefficients = *cutCoefficients[0];
            leftLowCut.setBypassed<3>(false);
            break;
        default:
            break;
    }
    
    //repeat for right chain
    
    auto &rightLowCut = rightChain.get<ChainPositions::LowCut>();
    //we need to bypass all the links in the chain first
    rightLowCut.setBypassed<0>(true);
    rightLowCut.setBypassed<1>(true);
    rightLowCut.setBypassed<2>(true);
    rightLowCut.setBypassed<3>(true);
    //now we can switch between slopes with a switch case
    switch (chainSettings.lowCutSlope)
    {
        case Slope_12:
            //assign coefficient to first filter
            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
            //stop bypassing first filter
            rightLowCut.setBypassed<0>(false);
            //etc
            break;
        case Slope_24:
            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<0>(false);
            *rightLowCut.get<1>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<1>(false);
            break;
        case Slope_36:
            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<0>(false);
            *rightLowCut.get<1>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<1>(false);
            *rightLowCut.get<2>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<2>(false);
            break;
        case Slope_48:
            *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<0>(false);
            *rightLowCut.get<1>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<1>(false);
            *rightLowCut.get<2>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<2>(false);
            *rightLowCut.get<3>().coefficients = *cutCoefficients[0];
            rightLowCut.setBypassed<3>(false);
            break;
        default:
            break;
    }
    
    
    //the processorchain requires a processing context to run audio through the chain
    //we need to extract left channel and right channel from the block given from the DAW
    //initializing a block with our buffer
    juce::dsp::AudioBlock<float> block(buffer);
    //now we extract individual channels
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    //now we can create processing contexts for each channel's blocks
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
    //now we can pass these contexts to our processing chains
    leftChain.process(leftContext);
    rightChain.process(rightContext);
    //now audio is running through our chains and being processed, but we have not yet set up filter params
    //we're goint to set up the parameters starting at the top of .h with a struct to stay organized
}

//==============================================================================
bool SimpleEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleEQAudioProcessor::createEditor()
{
    //return new SimpleEQAudioProcessorEditor (*this);
    
    //this allows us to see the plugin with parameters even without adding gui elements
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SimpleEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;
    
    //get values from apvts
    settings.lowCutFreq = apvts.getRawParameterValue("LowCutFreq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HiCutFreq")->load();
    settings.peakFreq = apvts.getRawParameterValue("PeakFreq")->load();
    settings.peakGainInDb = apvts.getRawParameterValue("PeakGain")->load();
    settings.peakQuality = apvts.getRawParameterValue("PeakQ")->load();
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCutSlope")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HiCutSlope")->load());
    
    return settings;
    
    //now that we have these settings we can create our filters' coefficients in preparetoplay
}

//DECLARING THE AUDIOPROCESSORVALUETREESTATE PARAMETER LAYOUT
//WE DECLARE THE PARAMETERS WE WILL USE IN HERE
juce::AudioProcessorValueTreeState::ParameterLayout SimpleEQAudioProcessor::createParameterLayout()
{
    //first we create the layout
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    //then we can add parameters to the layout
    
    //this one is a float parameter, we set the name, range, step size, skew amount if we want it, and default value
    //these should be unique pointers so we use std::make_unique
    //this parameter is for the lowcut filter
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("LowCutFreq", 1), "LowCut Freq", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, .25f), 20.f));
    //this parameter is for the highcut filter
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("HiCutFreq", 1), "HiCut Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, .25f), 20000.f));
    //this is for the peak EQ frequency
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("PeakFreq", 1), "Peak Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, .25f), 750.f));
    //this is for peak EQ gain - the range will now be in DB rather than hz, .1db steps, default value 0
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("PeakGain", 1), "Peak Gain", juce::NormalisableRange<float>(-24.f, 24.f, .1f, 1.), 0.f));
    //this is for the Q of the peak EQ
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("PeakQ", 1), "Q",
        juce::NormalisableRange<float>(.1f, 10.f, .05f, 1.), 1.f));
    
    //our low and high cut bands will have 4 choices of steepness to their cutoff
    //so we create a JUCE choice parameter which takes a string array with your choices
    
    //creating the string array of choices
    juce::StringArray cutoffChoiceStringArray;
    for (int i = 0; i < 4; i++)
    {
        juce::String str; //make a new string
        str << (12 + i*12); //12, 24, 36, 48
        str << " db/Oct"; //append with db/oct
        cutoffChoiceStringArray.add(str); //add to array
    }
    
    //creating the audioparameter choices
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("LowCutSlope", 1), "LowCut Slope", cutoffChoiceStringArray, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("HiCutSlope", 1), "HiCut Slope", cutoffChoiceStringArray, 0));
    
    return layout;
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleEQAudioProcessor();
}
