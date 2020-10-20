//
// Created by sufy on 22/11/2018.
//
#include "AudioEngine.h"
#include <android/log.h>
#include <oboe/Oboe.h>
#include <thread>
#include <array>
#include <mutex>
#include <string>
#include <cmath>
#include <math.h>
#include <sstream>
#include "../../../../../oboe/src/common/OboeDebug.h"
#include "Oscillator.h"
#include "../../../../../oboe/src/common/Trace.h"
#include <memory>
//#i latency and protection against glitches.
constexpr int32_t kBufferSizeInBursts = 2;

using namespace oboe;

DataCallbackResult
AudioEngine::onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numframes) {

    int32_t bufferSize = stream->getBufferSizeInFrames();

    if (mBufferSizeSelection == kBufferSizeAutomatic) {
        mLatencyTuner->tune();
    } else if (bufferSize != (mBufferSizeSelection * mFramesPerBurst)) {
        auto setBufferResult = stream->setBufferSizeInFrames(mBufferSizeSelection * mFramesPerBurst);
        if (setBufferResult == oboe::Result::OK) bufferSize = setBufferResult.value();
    }

    /**
     * The following output can be seen by running a systrace. Tracing is preferable to logging
     * inside the callback since tracing does not block.
     *
     * See https://developer.android.com/studio/profile/systrace-commandline.html
     */
    auto underrunCountResult = stream->getXRunCount();

    Trace::beginSection("numFrames %d, Underruns %d, buffer size %d",
                        numframes, underrunCountResult.value(), bufferSize);
    bool is16BitFormat = (stream->getFormat() == oboe::AudioFormat::I16);
    float *outputBuffer = (is16BitFormat) ? mConversionBuffer.get()
                                          : static_cast<float *>(audioData);
    music.render(outputBuffer, numframes);
    if (is16BitFormat) {
        oboe::convertFloatToPcm16(outputBuffer,
                                  static_cast<int16_t *>(audioData),
                                  numframes * channels);
    }

    if (mIsLatencyDetectionSupported) {
        calculateCurrentOutputLatencyMillis( &mCurrentOutputLatencyMillis);
    }

    Trace::endSection();

    return DataCallbackResult::Continue;
}
void AudioEngine::onErrorBeforeClose(AudioStream *oboeStream, Result error) {


    AudioStreamCallback::onErrorAfterClose(oboeStream, error);
}
void AudioEngine::setupPlaybackStreamParameters(oboe::AudioStreamBuilder *builder) {
    // builder->setAudioApi(mAudioApi);
    //builder->setDeviceId(mPlaybackDeviceId);
    builder->setChannelCount(2);
    builder->setAudioApi(AudioApi::OpenSLES)
    // We request EXCLUSIVE mode since this will give us the lowest possible latency.
    // If EXCLUSIVE mode isn't available the builder will fall back to SHARED mode.
    builder->setSharingMode(oboe::SharingMode::Exclusive);
    builder->setPerformanceMode(oboe::PerformanceMode::LowLatency);
    builder->setCallback(this);
}
void AudioEngine::setChannelCount(int channelCount) {

    if (channelCount != mChannelCount) {
        LOGD("Setting channel count to %d", channelCount);
        mChannelCount = channelCount;
        restartStream();
    } else {
        LOGW("Channel count was already %d, not setting", channelCount);
    }
}
void AudioEngine::createPlaybackStream() {

    oboe::AudioStreamBuilder builder;
    setupPlaybackStreamParameters(&builder);

    oboe::Result result = builder.openStream(&stream);

    if (result == oboe::Result::OK && stream != nullptr) {

        mSampleRate = stream->getSampleRate();
        mFramesPerBurst = stream->getFramesPerBurst();

        int channelCount = stream->getChannelCount();
        if (channelCount != mChannelCount){
            LOGW("Requested %d channels but received %d", mChannelCount, channelCount);
        }

        // Set the buffer size to the burst size - this will give us the minimum possible latency
        stream->setBufferSizeInFrames(mFramesPerBurst);

        // TODO: Implement Oboe_convertStreamToText
        // PrintAudioStreamInfo(mPlayStream);
        if (stream->getFormat() == oboe::AudioFormat::I16){
            // create a buffer of floats which we can render our audio data into
            int conversionBufferSamples = stream->getBufferCapacityInFrames() * channelCount;
            LOGD("Stream format is 16-bit integers, creating a temporary buffer of %d samples"
                 " for float->int16 conversion", conversionBufferSamples);
            mConversionBuffer = std::make_unique<float[]>(conversionBufferSamples);
        }

        mSampleRate=stream->getSampleRate();

        channels=stream->getChannelCount();
        channelCount=channels;


        // Create a latency tuner which will automatically tune our buffer size.
        mLatencyTuner = std::make_unique<oboe::LatencyTuner>(*stream);
        doStuff();
        // Start the stream - the dataCallback function will start being called
        result = stream->requestStart();
        if (result != oboe::Result::OK) {
            LOGE("Error starting stream. %s", oboe::convertToText(result));
        }

        mIsLatencyDetectionSupported = (stream->getTimestamp(CLOCK_MONOTONIC, 0, 0) !=
                                        oboe::Result::ErrorUnimplemented);

    } else {
        LOGE("Failed to create stream. Error: %s", oboe::convertToText(result));
    }

}
oboe::Result
AudioEngine::calculateCurrentOutputLatencyMillis(
        double *latencyMillis) {

    // Get the time that a known audio frame was presented for playing
    auto result = stream->getTimestamp(CLOCK_MONOTONIC);

    if (result == oboe::Result::OK) {

        oboe::FrameTimestamp playedFrame = result.value();

        // Get the write index for the next audio frame
        int64_t writeIndex = stream->getFramesWritten();

        // Calculate the number of frames between our known frame and the write index
        int64_t frameIndexDelta = writeIndex - playedFrame.position;

        // Calculate the time which the next frame will be presented
        int64_t frameTimeDelta = (frameIndexDelta * oboe::kNanosPerSecond) / mSampleRate;
        int64_t nextFramePresentationTime = playedFrame.timestamp + frameTimeDelta;

        // Assume that the next frame will be written at the current time
        using namespace std::chrono;
        int64_t nextFrameWriteTime =
                duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();

        // Calculate the latency
        *latencyMillis = static_cast<double>(nextFramePresentationTime - nextFrameWriteTime)
                         / kNanosPerMillisecond;
    } else {
        LOGE("Error calculating latency: %s", oboe::convertToText(result.error()));
    }

    return result;
}

AudioEngine::~AudioEngine() {

    closeOutputStream();
}

void AudioEngine::closeOutputStream() {

    if (stream != nullptr) {
        oboe::Result result = stream->requestStop();
        if (result != oboe::Result::OK) {
            LOGE("Error stopping output stream. %s", oboe::convertToText(result));
        }

        result = stream->close();
        if (result != oboe::Result::OK) {
            LOGE("Error closing output stream. %s", oboe::convertToText(result));
        }
    }
}
bool AudioEngine::start() {
    oboe::AudioStreamBuilder builder;
    builder.setCallback(this);
    builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
    builder.setSharingMode(SharingMode::Exclusive);

    Result result = builder.openStream(&stream);
    doStuff();
    LOGE("done stuff");
    channels = stream->getChannelCount();
    if (stream->getFormat() == oboe::AudioFormat::I16) {
        // create a buffer of floats which we can render our audio data into
        int conversionBufferSamples = stream->getBufferCapacityInFrames() * channels;
        LOGD("Stream format is 16-bit integers, creating a temporary buffer of %d samples"
             " for float->int16 conversion", conversionBufferSamples);
        mConversionBuffer = std::make_unique<float[]>(conversionBufferSamples);
    }
    if (result != Result::OK) {
        LOGE("Error opening stream: %s", convertToText(result));
        return false;
    }
    auto setBufferSizeResult = stream->setBufferSizeInFrames(stream->getFramesPerBurst() * 2);
    if (setBufferSizeResult) {
        LOGD("New Buffer size is %d frames", setBufferSizeResult);
    }

    mLatencyTuner = std::make_unique<oboe::LatencyTuner>(*stream);
    result = stream->requestStart();
    if (result != Result::OK) {
        LOGE("Error starting stream: %s", convertToText(result));
        return false;
    }


    mIsLatencyDetectionSupported = (stream->getTimestamp(CLOCK_MONOTONIC, 0, 0) !=
                                    oboe::Result::ErrorUnimplemented);

    // Sets the buffer size.

    // Starts the stream.



    return true;
}
void AudioEngine::setDeviceId(int32_t deviceId) {

    mPlaybackDeviceId = deviceId;

    // If this is a different device from the one currently in use then restart the stream
    int32_t currentDeviceId = stream->getDeviceId();
    if (deviceId != currentDeviceId) restartStream();
}
void AudioEngine::restart(){

    static std::mutex restartingLock;
    if (restartingLock.try_lock()){
        stop();
        start();
        restartingLock.unlock();
    }
}

void AudioEngine::stop() {
    if (stream != nullptr)
        stream->close();


}

void AudioEngine::doStuff() {

    LOGE("channels %d", channels);
    music.setChannels(channels);
    oscillator_.m_uWaveform = 1;
    oscillator0_.m_uWaveform = 1;
    oscillator1_.m_uWaveform = 1;
    oscillator2_.m_uWaveform = 1;
    oscillator2_.m_uWaveform = 0;
    drone = 0;
    oscillator_.createWaveTables();
    oscillator0_.createWaveTables();
    oscillator1_.createWaveTables();
    oscillator2_.createWaveTables();
    oscillator3_.createWaveTables();
    oscillator_.update();
    oscillator0_.update();
    oscillator0_.m_nCents = -2.5;
    oscillator1_.update();
    oscillator1_.m_nCents = 2.2;
    oscillator2_.update();
    oscillator2_.m_nCents=1;
    oscillator3_.update();
    oscillator3_.m_nCents=-1;

    music.setOsc(&oscillator_, &oscillator0_, &oscillator1_, &oscillator2_,&oscillator3_);
    music.setDelay(&delay);
    music.setEnv(&env);
    music.setDCA(&dca);
    music.setLFO(&lfo);
    music.setModMatrix(&m_GlobalModMatrix);
    music.setFilter(&filter);

    // Opens the stream.


    // Retrieves the sample rate of the stream for our oscillator.
    int sampleRate = stream->getSampleRate();

    LOGE("sample rate %d", sampleRate);
    reverb.prepareReverb(sampleRate);
    music.setReverb(&reverb);
    oscillator_.setSampleRate(sampleRate);
    oscillator0_.setSampleRate(sampleRate);
    oscillator1_.setSampleRate(sampleRate);
    oscillator2_.setSampleRate(sampleRate);
    oscillator3_.setSampleRate(sampleRate);
    filter.setSampleRate(sampleRate);
    lfo.setSampleRate(sampleRate);
    lfo.startOscillator();
    lfo.oscfc = 0;
    lfo.m_dOscFo = 0;
    env.setSampleRate(sampleRate);
    dca.setAmplitude_dB(100);
    dca.setEGMod(1);
    dca.setPanControl(0.1);
    delay.prepareForPlay(sampleRate);
    delay.setDelayTime_mSec(0);
    delay.setFeedback_Pct(0);
    delay.setDelayRatio(0);
    delay.setWetMix(0);
    delay.setMode(3);
    delay.update();
    ChangeKnob(15,0);

}
void AudioEngine::onErrorAfterClose(oboe::AudioStream *oboeStream, oboe::Result error) {
    if (error == oboe::Result::ErrorDisconnected) restartStream();
}

void AudioEngine::restartStream() {

    LOGI("Restarting stream");

    if (mRestartingLock.try_lock()) {
        closeOutputStream();
        createPlaybackStream();
        mRestartingLock.unlock();
    } else {
        LOGW("Restart stream operation already in progress - ignoring this request");
        // We were unable to obtain the restarting lock which means the restart operation is currently
        // active. This is probably because we received successive "stream disconnected" events.
        // Internal issue b/63087953
    }
}

double AudioEngine::getCurrentOutputLatencyMillis() {
    return mCurrentOutputLatencyMillis;
}

void AudioEngine::setBufferSizeInBursts(int32_t numBursts) {
    mBufferSizeSelection = numBursts;
}

bool AudioEngine::isLatencyDetectionSupported() {
    return mIsLatencyDetectionSupported;
}

void AudioEngine::setAudioApi(oboe::AudioApi audioApi) {
    if (audioApi != mAudioApi) {
        LOGD("Setting Audio API to %s", oboe::convertToText(audioApi));
        mAudioApi = audioApi;
        restartStream();
    } else {
        LOGW("Audio API was already set to %s, not setting", oboe::convertToText(audioApi));
    }
}

void AudioEngine::setToneOn(bool isToneOn) {
    // env.startEG();
    if(isToneOn) {
        oscillator_.startOscillator();
        oscillator0_.startOscillator();
        oscillator1_.startOscillator();
        oscillator2_.startOscillator();
        oscillator3_.startOscillator();
        env.startEG();
        // oscillator_.m_bNoteOn=true;
    }
    else {
        // oscillator_.stopOscillator();
        // oscillator_.m_bNoteOn=false;
        __android_log_print(ANDROID_LOG_ERROR, "tagtest","data:%d",drone);

        if(drone==0)
            env.noteOff();
    }
}

void AudioEngine::setNote(int note) {
    //oscillator_.setNote(getHertz(note));
    oscillator_.m_dOscFo = getHertz(note );
    oscillator0_.m_dOscFo=getHertz(note );
    oscillator1_.m_dOscFo=getHertz(note );
    oscillator2_.m_dOscFo=getHertz(note );

    oscillator3_.m_dOscFo=getHertz(note);


    oscillator_.m_uMIDINoteNumber = getHertz(note);
    oscillator0_.m_uMIDINoteNumber=getHertz(note);
    oscillator1_.m_uMIDINoteNumber=getHertz(note);
    oscillator2_.m_uMIDINoteNumber=getHertz(note);
    oscillator3_.m_uMIDINoteNumber=getHertz(note);
    __android_log_print(ANDROID_LOG_ERROR, "tagtest","note : %d  : Freq %f",note, oscillator_.m_dOscFo);
    oscillator_.update();
    oscillator0_.update();
    oscillator1_.update();
    oscillator2_.update();
    oscillator3_.update();

}
float AudioEngine::getHertz(int keyNumber) {
    return pow(2, (keyNumber - 69) / 12.0) * basehertz;
}


void AudioEngine::UpdateEnv() {
    env.setAttackTime_mSec(500);
    env.setDecayTime_mSec(200);
    env.setSustainLevel(1);
    env.setReleaseTime_mSec(3000);

    env.m_bResetToZero = false;
    env.m_bLegatoMode = false;
}
void AudioEngine::ChangeKnobDouble(int knob , double val) {
    if(knob==16)
        basehertz=val;

}
void AudioEngine::ChangeKnob(int knob , int val) {

    if(knob==0) {
        env.setAttackTime_mSec( (double)val*100);
        //  env.update();
    }
    if(knob==1) {
        env.setDecayTime_mSec((double)val *100);
        // env.update();
    }
    if(knob==2) {
        env.setSustainLevel((double)val/50);
        // env.update();
    }
    if(knob==3) {
        env.setReleaseTime_mSec((double)val*300);
        // env.update();
    }
    if(knob==4) {
        filter.m_dFcControl = (double) pow(2,(float) val/10/12/1.5) * 440 ;
        float mult=4;
        music.filterfc =  filter.m_dFcControl;
        //   __android_log_print(ANDROID_LOG_ERROR, "tagtestosc","data:%f",(double)music.filterfc);
//       filter.update();
        //   __android_log_print(ANDROID_LOG_ERROR, "tagtestfilter", "data:%f", filter.m_dFcControl);
    }
    if(knob==5) {
        filter.m_dQControl = (double) val / 10;
        //    __android_log_print(ANDROID_LOG_ERROR, "tagtestfilter", "data:%f", (double) val / 10);
//      filter.update();
    }
    if(knob==6) {
        delay.setWetMix((double)val/50);
        delay.update();
    }
    if(knob==7) {
        delay.setDelayTime_mSec((double)val*5*8);
        //    LOGE("delay tune %f",((double)val*5*8));
        delay.update();
    }
    if(knob==8) {
        if(val<0)
            delay.setDelayRatio((((double)val/55)*2-0.9)/2-0.5);
        else
            delay.setDelayRatio((((double)val/55)*2-0.9)/2+0.5);
        //LOGE("delay ratio %f",(((double)val/55)*2-0.9)/2);
        delay.update();
    }
    if(knob==9) {
        delay.setFeedback_Pct((double)val*2);
        delay.update();
    }
    if(knob==10) {
        lfo.m_dOscFo=exp((double)(val/10)/20)*(20 / (exp(5) ));
        //   __android_log_print(ANDROID_LOG_ERROR, "tagtestosc","data:%f",exp((double)((double)val/10)/20)*(20 / (exp(5) )));

    }
    if(knob==11) {
        double num = (double) val/15;

        lfo.oscfc = (double) num - (double) num / 2;

        //    __android_log_print(ANDROID_LOG_ERROR, "tagtestosc","data:%f",(double)lfo.oscfc);
    }
    if(knob==12) {
        lfo.filterFc = (double) val;


        //    __android_log_print(ANDROID_LOG_ERROR, "tagtestfiltfc","data:%f",lfo.filterFc);
    }
    if(knob==13) {
        drone=val;
    }
    if(knob==14) {
       reverb.m_fWet_pct=val;
    }
    if(knob==15) {
        __android_log_print(ANDROID_LOG_ERROR, "wave","data:%d",val);
        oscillator3_.m_uWaveform=0;
        oscillator3_.m_nCents=0;
        oscillator3_.m_nSemitones=0;
        oscillator3_.m_nCents=0;
        oscillator2_.m_uWaveform=0;
        oscillator2_.m_nCents=0;
        oscillator2_.m_nSemitones=0;
        oscillator2_.m_nCents=0;
        oscillator1_.m_uWaveform=0;
        oscillator1_.m_nCents=0;
        oscillator1_.m_nSemitones=0;
        oscillator1_.m_nCents=0;
        oscillator0_.m_uWaveform=0;
        oscillator0_.m_nCents=0;
        oscillator0_.m_nSemitones=0;
        oscillator0_.m_nCents=0;
        oscillator_.m_uWaveform=0;
        oscillator_.m_nCents=0;
        oscillator_.m_nSemitones=0;
        oscillator_.m_nCents=0;
        if(val==2) {
            oscillator0_.m_uWaveform=0;

            oscillator_.m_uWaveform=0;
            oscillator_.m_nOctave=1;
            oscillator1_.m_uWaveform=0;
            oscillator2_.m_nCents=12;
            oscillator2_.m_uWaveform=0;
            oscillator3_.m_uWaveform=0;
            oscillator3_.m_nSemitones=7;




            music.fmsynth=true;
        } else {
            oscillator_.setPhaseMod(0);
            oscillator1_.setPhaseMod(0);
            oscillator0_.setPhaseMod(0);
            oscillator2_.setPhaseMod(0);
            oscillator3_.setPhaseMod(0);
            music.fmsynth=false;
            oscillator1_.m_nOctave=0;
        }
        if(val==1) {
            oscillator0_.m_uWaveform=4;
            oscillator1_.m_uWaveform=3;
            oscillator_.m_uWaveform=4;

            oscillator1_.m_nSemitones=-12;

            oscillator2_.m_uWaveform=4;
            oscillator_.m_nCents=-7;
            oscillator2_.m_nCents=7;


            music.fmsynth=false;
        }
        if(val==0) {

            oscillator1_.m_uWaveform=3;

            oscillator_.m_uWaveform=3;
            oscillator0_.m_uWaveform=0;

            oscillator2_.m_uWaveform=3;

            oscillator1_.m_uWaveform=3;
            oscillator_.m_nCents=-7;
            oscillator0_.m_nOctave=-1;
            oscillator2_.m_nCents=7;
            music.fmsynth=false;
        }
        if(val==3) {

            oscillator_.m_nCents=9;
            oscillator0_.m_uWaveform=5;
            oscillator0_.m_nSemitones=5;
            oscillator0_.m_nCents=5;

            oscillator1_.m_uWaveform=5;
            oscillator1_.m_nOctave=-1;

            oscillator2_.m_uWaveform=5;
            oscillator2_.m_nCents=-9;
            oscillator3_.m_uWaveform=7;
            music.fmsynth=false;
        }
        if(val==4) {
            oscillator0_.m_uWaveform=7;
            oscillator1_.m_uWaveform=1;
            oscillator1_.m_nSemitones=7;

            oscillator_.m_uWaveform=7;
            oscillator_.m_nOctave=1;
            oscillator2_.m_uWaveform=5;
            oscillator2_.m_nOctave=-1;
            oscillator3_.m_uWaveform=0;
            music.fmsynth=false;
        }
    }if(knob==16) {
        if(val==0) {
            oscillator_.stopOscillator();

        }
    }

}


