#pragma once
#include <vector>
#include <string>

class AudioProcessor {
public:
    struct AudioData {
        std::vector<double> samples;
        double sampleRate;
        double threshold;
        int minDistance;
        std::vector<size_t> peaks;
    };

    static AudioData processWavFile(const std::string& filePath, 
                                  double thresholdMultiplier,
                                  double minLength);
                                  
private:
    static std::vector<size_t> customFindPeaks(const std::vector<double>& data,
                                             double threshold,
                                             size_t minDistance);
};