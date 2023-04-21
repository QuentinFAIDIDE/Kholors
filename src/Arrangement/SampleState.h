#ifndef DEF_SAMPLE_STATE_HPP
#define DEF_SAMPLE_STATE_HPP

class SampleState: public Marshalable, public State {
    int id;
    std::string samplePath;
    int64_t editingPosition;
    int64_t bufferPosition;
    int64_t length;
    int64_t lowPassRepeat;
    int64_t highPassRepeat;
    std::vector<std::pair<int64_t, int> lowPassFreqsAutomation;
    std::vector<std::pair<int64_t, int> highPassFreqsAutomation;
};

#endif // DEF_SAMPLE_STATE_HPP