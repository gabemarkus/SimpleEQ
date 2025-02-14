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
    //we need to create a ProcessSpec to tell our chain how it will process audio
    
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock; //how many samples per block/how many it will process at once
    spec.numChannels = 1; //how many channels to process
    spec.sampleRate = sampleRate; //sample rate
    
    //now we pass this spec to each chain
    leftChain.prepare(spec);
    rightChain.prepare(spec);
    
    UpdateAllFilters();
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
    
    //1. Update filter coefficients based on knob parameters
    UpdateAllFilters();
    
    //the processorchain requires a processing context to run audio through the chain
    //we need to extract left channel and right channel from the block given from the DAW
    //2. Initializing a block with our buffer
    juce::dsp::AudioBlock<float> block(buffer);
    //3. Extract individual channels from block
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    //4. Create Processing Context for each channel
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
    //5. Process left and right hain
    leftChain.process(leftContext);
    rightChain.process(rightContext);
}

//==============================================================================
bool SimpleEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleEQAudioProcessor::createEditor()
{
    return new SimpleEQAudioProcessorEditor (*this);
    
    //this allows us to see the plugin with parameters even without adding gui elements
    //return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    //we are creating a way to save parameters when opening and closing the plugin
    juce::MemoryOutputStream memoryOutputStream (destData, true);
    apvts.state.writeToStream(memoryOutputStream);
}

void SimpleEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{  
    //here we restore the saved parameters from memory
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if(tree.isValid())
    {
        apvts.replaceState(tree);
        UpdateAllFilters();
    }
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
    settings.lowCutBypassed = apvts.getRawParameterValue("LowCutBypassed")->load() > 0.5f;
    settings.highCutBypassed = apvts.getRawParameterValue("HighCutBypassed")->load() > 0.5f;
    settings.peakBypassed = apvts.getRawParameterValue("PeakBypassed")->load() > 0.5f;
    
    return settings;
}

Coefficients MakePeakFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, chainSettings.peakFreq, chainSettings.peakQuality, juce::Decibels::decibelsToGain(chainSettings.peakGainInDb));
}

void UpdateCoefficients(Coefficients &old, const Coefficients &replacement)
{
    *old = *replacement;
}

void SimpleEQAudioProcessor::UpdateLowCutFilters(const ChainSettings &chainSettings)
{
    auto lowCutCoefficients = MakeLowCutFilter(chainSettings, getSampleRate());
    //remember we need to split channels
    auto &leftLowCut = leftChain.get<ChainPositions::LowCut>();
    auto &rightLowCut = rightChain.get<ChainPositions::LowCut>();
    
    leftChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    rightChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    
    UpdateCutFilter(leftLowCut, lowCutCoefficients, static_cast<Slope>(chainSettings.lowCutSlope));
    UpdateCutFilter(rightLowCut, lowCutCoefficients, static_cast<Slope>(chainSettings.lowCutSlope));
}

void SimpleEQAudioProcessor::UpdateHighCutFilters(const ChainSettings &chainSettings)
{
    auto highCutCoefficients = MakeHighCutFilter(chainSettings, getSampleRate());
    auto &leftHighCut = leftChain.get<ChainPositions::HighCut>();
    auto &rightHighCut = rightChain.get<ChainPositions::HighCut>();
    
    leftChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);
    rightChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);
    
    UpdateCutFilter(leftHighCut, highCutCoefficients, static_cast<Slope>(chainSettings.highCutSlope));
    UpdateCutFilter(rightHighCut, highCutCoefficients, static_cast<Slope>(chainSettings.highCutSlope));
}

void SimpleEQAudioProcessor::UpdatePeakFilter(const ChainSettings& chainSettings)
{
    auto peakCoefficients = MakePeakFilter(chainSettings, getSampleRate());
    
    leftChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);
    rightChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);
    
    UpdateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    UpdateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
}

void SimpleEQAudioProcessor::UpdateAllFilters()
{
    auto chainSettings = getChainSettings(apvts);
    UpdateLowCutFilters(chainSettings);
    UpdatePeakFilter(chainSettings);
    UpdateHighCutFilters(chainSettings);
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
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("PeakGain", 1), "Peak Gain", juce::NormalisableRange<float>(-24.f, 24.f, .5f, 1.), 0.f));
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
    
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("LowCutBypassed", 1), "LowCut Bypassed", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("PeakBypassed", 1), "Peak Bypassed", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("HighCutBypassed", 1), "HighCut Bypassed", false));

    return layout;
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleEQAudioProcessor();
}
