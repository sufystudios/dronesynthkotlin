//
// Created by sufy on 22/11/2018.
//

#ifndef WAVEMAKER_AUDIOENGINE_H
#define WAVEMAKER_AUDIOENGINE_H

#include <oboe/Oboe.h>

#include <math.h>
#include "Oscillator.h"
//#include "QBLimitedOscillator.h"
#include "EnvelopeGenerator.h"
#include "MusicSystem.h"
#include "DCA.h"
#include "ModulationMatrix.h"
#include "MoogLadderFilter.h"
#include "LFO.h"
#include "StereoDelayFX.h"
#include "WTOscillator.h"
#include "../../../../../oboe/src/common/Trace.h"

constexpr int32_t kBufferSizeAutomatic = 0;
using namespace oboe;

class AudioEngine : public AudioStreamCallback {
public:
    bool start();
    void setBufferSizeInBursts(int32_t numBursts);
    std::unique_ptr<float[]> mConversionBuffer{nullptr};
    AudioStream *stream = nullptr;
    void closeOutputStream();
    DataCallbackResult onAudioReady(AudioStream *, void *, int32_t);
    void stop();
    void restart();
    AudioEngine() {
        Trace::initialize();
        createPlaybackStream();
        doStuff();
    }
    void restartStream();
    void onErrorAfterClose(oboe::AudioStream *oboeStream, oboe::Result error);
    bool fmsynth = false;


    void setChannelCount(int channelCount);
    double getCurrentOutputLatencyMillis();

    bool isLatencyDetectionSupported();
    void setDeviceId(int32_t deviceId);

    void setAudioApi(oboe::AudioApi audioApi);
    oboe::Result calculateCurrentOutputLatencyMillis( double *latencyMillis);
    void onErrorBeforeClose(AudioStream *oboeStream, Result error) override;
    CDelayLine looperdelay;
    void setToneOn(bool isToneOn);
    void setNote(int note);
    double basehertz=440.0;
    float getHertz(int key);
    void UpdateEnv() ;
    void ChangeKnob(int, int);
    void ChangeKnobDouble(int, double);
    int32_t mBufferSizeSelection = kBufferSizeAutomatic;
    bool mIsLatencyDetectionSupported = false;
    std::unique_ptr<oboe::LatencyTuner> mLatencyTuner;
    std::mutex mRestartingLock;
    ~AudioEngine();
    void doStuff();
    void tubeon(int tube) {


        music.tubeon=tube;

    }
    bool istoneon=false;
private:

    CEnvelopeGenerator env;
    oboe::AudioApi mAudioApi = oboe::AudioApi::Unspecified;
    int32_t mPlaybackDeviceId = oboe::kUnspecified;
    int32_t mSampleRate;
    int32_t mChannelCount = 2; // Stereo
    void setupPlaybackStreamParameters(oboe::AudioStreamBuilder *builder);
    int32_t mFramesPerBurst;
    void createPlaybackStream();
    double mCurrentOutputLatencyMillis = 0;
    CModulationMatrix m_GlobalModMatrix;
    void setLoop() {

        music.looper=!music.looper;
    }
    CWTOscillator oscillator_;
    CWTOscillator oscillator0_;
    CWTOscillator oscillator1_;
    CWTOscillator oscillator2_;
    CWTOscillator oscillator3_;


//    CQBLimitedOscillator oscillator_;
//    CQBLimitedOscillator oscillator0_;
//    CQBLimitedOscillator oscillator1_;
    int channels = 2;
    MusicSystem music;
    CDCA dca;
    CMoogLadderFilter filter, filter2;
    CLFO lfo;
    CStereoDelayFX delay;
    // need these for mod matrix
    double m_dDefaultModIntensity;	// 1.0
    double m_dDefaultModRange;		// 1.0
    double m_dOscFoModRange;
    double m_dFilterModRange;
    bool random_lfo=false;
    int drone=0;
    // --- NS MM2 Additions
    double m_dOscFoPitchBendModRange;
    double m_dAmpModRange;
    UINT m_uOscWaveform;
    enum{SINE,SAW1,SAW2,SAW3,TRI,SQUARE,NOISE,PNOISE};
    double m_dLFO1Rate;
    double m_dLFO1Amplitude;
    UINT m_uLFO1Waveform;
    enum{sine,usaw,dsaw,tri,square,expo,rsh,qrsh};
    UINT m_uLFO1Mode;
    enum{sync,shot,free};
    UINT m_uLFO1Destination;
    enum{Osc,Filter};
    double m_dFcControl;
    double m_dQControl;
    double m_dPanControl;
    double m_dAttackTime_mSec;
    double m_dDecayTime_mSec;
    double m_dSustainLevel;
    double m_dReleaseTime_mSec;
    double m_dEG1DCAIntensity;
    double m_dEG1OscIntensity;
    UINT m_uLegatoMode;
    enum{mono,legato};

    void setSampleRates();
};
#endif //WAVEMAKER_AUDIOENGINE_H
