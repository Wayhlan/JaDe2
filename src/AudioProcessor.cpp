#include "AudioProcessor.h"
#include <sndfile.h>
#include <algorithm>
#include <cmath>

AudioProcessor::AudioData AudioProcessor::processWavFile(
    const std::string& filePath, 
    double thresholdMultiplier,
    double minLength) 
{
    SF_INFO sfInfo{};
    SNDFILE* file = sf_open(filePath.c_str(), SFM_READ, &sfInfo);
    if (!file) throw std::runtime_error("Failed to open audio file");

    std::vector<double> data(sfInfo.frames * sfInfo.channels);
    sf_read_double(file, data.data(), data.size());
    sf_close(file);

    // Convert to mono if stereo
    if (sfInfo.channels > 1) {
        std::vector<double> mono(sfInfo.frames);
        for (size_t i = 0; i < mono.size(); ++i) {
            mono[i] = data[i * sfInfo.channels];
        }
        data.swap(mono);
    }

    double mean = 0.0;
    for (double sample : data) mean += fabs(sample);
    mean /= data.size();

    double threshold = thresholdMultiplier * mean;
    size_t minDistance = static_cast<size_t>(minLength * sfInfo.samplerate);

    return {
        data,
        static_cast<double>(sfInfo.samplerate),
        threshold,
        minDistance,
        customFindPeaks(data, threshold, minDistance)
    };
}

std::vector<size_t> AudioProcessor::customFindPeaks(
    const std::vector<double>& data,
    double threshold,
    size_t minDistance) 
{
    std::vector<size_t> peaks;
    size_t i = 0;
    
    while (i < data.size()) {
        if (data[i] > threshold) {
            double peakVal = data[i];
            size_t peakIdx = i;
            
            while (i < data.size() && data[i] > threshold) {
                if (data[i] > peakVal) {
                    peakVal = data[i];
                    peakIdx = i;
                }
                ++i;
            }
            
            peaks.push_back(peakIdx);
            i = peakIdx + minDistance;
        } else {
            ++i;
        }
    }
    
    return peaks;
}