/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
//here we initialize our slider attachments
peakFreqSliderAttachment(audioProcessor.apvts, "PeakFreq", peakFreqSlider),
peakGainSliderAttachment(audioProcessor.apvts, "PeakGain", peakGainSlider),
peakQualitySliderAttachment(audioProcessor.apvts, "PeakQ", peakQualitySlider),
lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCutFreq", lowCutFreqSlider),
lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCutSlope", lowCutSlopeSlider),
highCutFreqSliderAttachment(audioProcessor.apvts, "HiCutFreq", highCutFreqSlider),
highCutSlopeSliderAttachment(audioProcessor.apvts, "HiCutSlope", highCutSlopeSlider)

{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    //making our knobs appear
    for(auto* comp : GetComps())
    {
        addAndMakeVisible(comp);
    }
    
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }
    setSize (600, 400);
    
    //starting the timer to check for param changes (60fps)
    startTimerHz(60);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->removeListener(this);
    }
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);
    
    //setting up to display response curve
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop((bounds.getHeight() * .33f));
    auto w = responseArea.getWidth();
    
    //get chain elements
    auto& lowcut = monochain.get<ChainPositions::LowCut>();
    auto& peak = monochain.get<ChainPositions::Peak>();
    auto& highcut = monochain.get<ChainPositions::HighCut>();
    
    auto sampleRate = audioProcessor.getSampleRate();
    
    //getting magnitudes from each filter
    //storing them in a vector
    std::vector<double> magnitudes;
    //changing the size of the vector to the width of the response curve display (1 pixel = 1 magnitude)
    magnitudes.resize(w);
    //iterate through each element of the vector and calculate magnitude at that frequency
    for (int i = 0; i < w; i++)
    {
        //we need a starting gain of 1
        double mag = 1.f;
        //call magnitude function for pixel
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);
        if(!monochain.isBypassed<ChainPositions::Peak>())
        {
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if(!lowcut.isBypassed<0>())
        {
            mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if(!lowcut.isBypassed<1>())
        {
            mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if(!lowcut.isBypassed<2>())
        {
            mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if(!lowcut.isBypassed<3>())
        {
            mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if(!highcut.isBypassed<0>())
        {
            mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if(!highcut.isBypassed<1>())
        {
            mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if(!highcut.isBypassed<2>())
        {
            mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if(!highcut.isBypassed<3>())
        {
            mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        
        //convert magnitude to decibels
        magnitudes[i] = Decibels::gainToDecibels(mag);
    }
    
    //convert vector of magnitudes to path so we can draw it
    //Path = juce - sequence of lines and curves that may either form a closed shape or be open-ended
    Path responseCurve;
    //get window max and min positions
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };
    //starting the curve
    responseCurve.startNewSubPath(responseArea.getX(), map(magnitudes.front()));
    //drawing rest of the curve
    for (size_t i = 1; i < magnitudes.size(); i++)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(magnitudes[i]));
    }
    
    g.setColour(Colours::orange);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);
    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));
    
}

void SimpleEQAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    //top third of display = graph
    //bottom 2/3 = knobs/controls
    
    //get bounds of window
    auto bounds = getLocalBounds();
    //removing area reserved for graph
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * .33f);

    //creating area for lowcut knob
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * .33f);
    //half the remaining .66 for high cut knob
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * .5f);
    
    //applying bounds to knobs
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * .66f));
    lowCutSlopeSlider.setBounds(lowCutArea);
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * .66f));
    highCutSlopeSlider.setBounds(highCutArea);
    
    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * .33f));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * .5f));
    peakQualitySlider.setBounds(bounds);
}

void SimpleEQAudioProcessorEditor::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void SimpleEQAudioProcessorEditor::timerCallback()
{
    if(parametersChanged.compareAndSetBool(false, true))
    {
        //if the parameters have changed then we change the curve
        //update the monochain
        auto chainSettings = getChainSettings(audioProcessor.apvts);
        auto peakCoefficients = MakePeakFilter(chainSettings, audioProcessor.getSampleRate());
        UpdateCoefficients(monochain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
        
        auto lowCutCoefficients = MakeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
        auto highCutCoefficients = MakeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
        
        UpdateCutFilter(monochain.get<ChainPositions::LowCut>(), lowCutCoefficients, static_cast<Slope>(chainSettings.lowCutSlope));
        UpdateCutFilter(monochain.get<ChainPositions::HighCut>(), highCutCoefficients, static_cast<Slope>(chainSettings.highCutSlope));
        
        //signal a repaint
        repaint();
    }
}


std::vector<juce::Component*> SimpleEQAudioProcessorEditor::GetComps()
{
    return
    {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &highCutSlopeSlider,
        &lowCutSlopeSlider
    };
}
