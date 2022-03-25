//
// Created by sufy on 17/12/2018.
//

#include <cstdint>
#include <android/log.h>
#include <locale>
//#include "QBLimitedOscillator.h"
#include "DCA.h"
#include "MoogLadderFilter.h"
#include "StereoDelayFX.h"
#include "LFO.h"

#include "CombFilter.h"
#include "Delay.h"
#include "DelayAPF.h"
#include "LPFCombFilter.h"
#include "OnePoleLPF.h"
#include "WTOscillator.h"
#include "../../../../../oboe/src/common/OboeDebug.h"
#include "Reverb.h"
#include "AudioEngine.h"

#ifndef WAVEMAKER_MUSICSYSTEM_H
#define WAVEMAKER_MUSICSYSTEM_H

#define AI_LFO_MODIFY 0.002
#endif //WAVEMAKER_MUSICSYSTEM_H

#define SATURATION_INTENSITY 0.5

class MusicSystem {

public :
    ~MusicSystem() {
        LOGE("deleting music system");
    };
#define lfoslow 6
#define lfoslow1 3
    Reverb reverb;
    Reverb reverb2;
    float lfofrequency = 0;
    bool noise = false;
#define AMPLITUDE 0.3
    CWTOscillator *oscillator_;
    CWTOscillator *oscillator0_;
    CWTOscillator *oscillator1_;
    CWTOscillator *oscillator2_;
    CWTOscillator *oscillator3_;

    CEnvelopeGenerator *env;
    bool fmsynth = false;
    CStereoDelayFX *delay;
    CModulationMatrix *globalmm;
    CDCA *dca;
    int note;
    CMoogLadderFilter *filter;
    CMoogLadderFilter *filter2;
    CLFO AILFO3;
    CLFO *lfoc;

    int channels = 2;

    void setSampleRate(int rate) {
        sampleRate = rate;
    }

    void setChannels(int c) {
        channels = c;
    }

    bool randomlfo = false;
    bool randomovertones = false;
////REVERB
    float m_fWet_pct;

    CDelayLine *delayloop;

    void setOsc(CWTOscillator *b, CWTOscillator *c, CWTOscillator *d, CWTOscillator *e,
                CWTOscillator *f) {
        oscillator_ = b;
        oscillator0_ = c;
        oscillator1_ = d;
        oscillator2_ = e;
        oscillator3_ = f;
    }

    void setFilter(CMoogLadderFilter *fil, CMoogLadderFilter *fil2) {
        filter = fil;
        filter2 = fil2;
//        filter->m_dSaturation=(2);
//        filter2->m_dSaturation=2;

    }

    void setNote(int n) {
        note = n;
    }

    void setModMatrix(CModulationMatrix *global) {
        globalmm = global;
    }

    void setEnv(CEnvelopeGenerator *enve) {
        env = enve;

        AIEnvelope1.setSampleRate(sampleRate);


        AILFO1.setSampleRate(sampleRate);
        AILFO2.setSampleRate(sampleRate);
        AILFO3.setSampleRate(sampleRate);
        AILFO1.startOscillator();
        AILFO2.startOscillator();
        AILFO3.startOscillator();
        AILFO1.m_uWaveform = 1;
        AILFO2.m_uWaveform = 0;
        AILFO3.m_uWaveform = 1;
        AIEnvelope2.setEGMode(0);
        AIEnvelope1.setEGMode(0);

        AILFO1.m_dOscFo = 0.1;
        AILFO2.m_dOscFo = 3;
        AILFO3.m_dOscFo = 0.4;

        AIEnvelope1.setAttackTime_mSec(2000);
        AIEnvelope1.setReleaseTime_mSec(2000);
        AIEnvelope1.setSustainLevel(1);
        AIEnvelope1.startEG();

        AIEnvelope2.setSampleRate(sampleRate);
        AIEnvelope1.setAttackTime_mSec(2000);
        AIEnvelope1.setReleaseTime_mSec(2000);
        AIEnvelope1.setSustainLevel(1);
        AIEnvelope1.startEG();


        AIEnvelope3.setSampleRate(sampleRate);
        AIEnvelope3.setAttackTime_mSec(1500);
        AIEnvelope3.setReleaseTime_mSec(2000);
        AIEnvelope3.setSustainLevel(1);
        AIEnvelope3.startEG();

        AIEnvelope4.setSampleRate(sampleRate);
        AIEnvelope4.setAttackTime_mSec(1500);
        AIEnvelope4.setReleaseTime_mSec(2000);
        AIEnvelope4.setSustainLevel(1);
        AIEnvelope4.startEG();

        AIEnvelope5.setSampleRate(sampleRate);
        AIEnvelope5.setAttackTime_mSec(1500);
        AIEnvelope5.setReleaseTime_mSec(2000);
        AIEnvelope5.setSustainLevel(1);
        AIEnvelope5.startEG();

        AIEnvelope1.setEGMode(0);
        AIEnvelope2.setEGMode(0);
        AIEnvelope3.setEGMode(0);
        AIEnvelope4.setEGMode(0);
        AIEnvelope5.setEGMode(0);


        delayEnvelope.setSampleRate(sampleRate);
        delayEnvelope.setAttackTime_mSec(1500);
        delayEnvelope.setReleaseTime_mSec(2000);
        delayEnvelope.setSustainLevel(1);
        delayEnvelope.startEG();
        delayEnvelope.setEGMode(0);

        filterEnvelope.setSampleRate(sampleRate);
        filterEnvelope.setAttackTime_mSec(200);
        filterEnvelope.setReleaseTime_mSec(2000);
        filterEnvelope.setSustainLevel(1);
        filterEnvelope.startEG();
        filterEnvelope.setEGMode(0);

    }

    CEnvelopeGenerator delayEnvelope;
    CEnvelopeGenerator filterEnvelope;

    bool ascending = true;

    void setDelay(CStereoDelayFX *del, CDelayLine *oop) {
        delay = del;
        delayloop = oop;
    }

    void setLFO(CLFO *lfo) {
        lfoc = lfo;
    }

    void setDCA(CDCA *dcal) {
        dca = dcal;
    }

    CEnvelopeGenerator AIEnvelope1;

    CEnvelopeGenerator AIEnvelope2;
    CEnvelopeGenerator AIEnvelope3;
    CEnvelopeGenerator AIEnvelope4;
    CEnvelopeGenerator AIEnvelope5;

    CLFO AILFO1;

    CLFO AILFO2;

    bool AIENVACTIVE = false;
    int timer = 0;
    int delayChanged = 0;
    enum State {
        PAUSE,
        FILTER_SWEEP,
        Q1,
        Q2,
        DELAY,
        OVERTONES,
        LFO_FILTER,
        LFO_SPEED,
        TUBE,
        NOISE_MIX,
        RANDOM_CENTS,
        RANDOM_FIFTH,
        SUB,
        RANDOM_EIGHTH
    };

    State currentState = PAUSE;
    State secondaryState;
    State trinaryState;
    int stateTimer = 0;

    double limitQ(double q) {
        double qMax = 8;
        if (q < 8.0)
            return q;
        if (q < 0)
            return 0;
        else return 8.0;
    }

    void performAI(float *data, int32_t numframes) {

        filt = 0;
        filt2 = 0;
        currentlfoslow = 0;
        oscilate = 0;
        oscilate0 = 0;
        oscilate1 = 0;

        oscilate2 = 0;
        oscilate3 = 0;

        combinedosc;
        feedback = 3;

        float en;
        float lfo;
        double AIEnvelopeOut1;
        double AIEnvelopeOut2;
        double AIEnvelopeOut3;
        double AIEnvelopeOut4;
        double AIEnvelopeOut5;
        double AILFOOut1;
        double AILFOOut2;
        double AILFOOut3;
        double filtAI;
        double AiFilter;
        double AiLfo;
        double AiQ1, AiQ2;
        double lfoFilter = 0;
        double lfoSpeed = 0;
        double filtEnvelope = 0;
        double currentFilt = 0;
        double currentQl = 0;
        double currentQr = 0;
        double AiTube = 0;
        double delayEnv = 0;
        int randomFifth = 0;
        int sub = 0;
        int randomEighth = 0;
        int randomNote = 0;
        CWTOscillator *notes[] = {oscillator_, oscillator0_, oscillator1_, oscillator2_,
                                  oscillator3_};

        CEnvelopeGenerator envelopes[] = {AIEnvelope1, AIEnvelope2, AIEnvelope3, AIEnvelope4,
                                          AIEnvelope5};
        for (int i = 0; i < numframes; i++) { ;


            if (stateTimer == 0) {


                if (currentState == PAUSE) {
                    currentState = static_cast<State >(rand() % 14);
                    do {
                        secondaryState = static_cast<State>(rand() % 10);
                    } while (secondaryState == currentState);
                    do {
                        trinaryState = static_cast<State>(rand() % 5);

                    } while (trinaryState == currentState || trinaryState == secondaryState);
                    __android_log_print(ANDROID_LOG_DEBUG, "state",
                                        "Current state %d, secondary state%d,trinary state%d",
                                        currentState, secondaryState, trinaryState);
                    if (randomNote < 5 && randomNote > 0) {
                        notes[randomNote]->m_nOctave = 0;
                        notes[randomNote]->m_nSemitones = 0;
                    }

                    randomNote = rand() % 5;
                    if (currentState == RANDOM_EIGHTH) {
                        notes[randomNote]->m_nOctave = rand() % 2;
                        envelopes[randomNote].reset();
                    } else if (currentState == RANDOM_FIFTH) {
                        notes[randomNote]->m_nSemitones = 7 + (12 * (rand() % 2));
                        envelopes[randomNote].reset();
                    } else if (currentState == SUB) {
                        notes[randomNote]->m_nOctave = -(rand() % 1);
                        envelopes[randomNote].reset();
                    }
                    if (currentState == LFO_FILTER || secondaryState == LFO_FILTER) {
                        lfoc->filterFc = rand() % 50;
                    }
                    if (currentState == DELAY || secondaryState == DELAY) {
                        delay->setMode(rand() % 3);
                    } else {
                        delay->setMode(2);
                    }

                    if (currentState == LFO_SPEED || secondaryState == LFO_SPEED)
                        AiLfo = exp((double) ((rand() + 30) % 55) / 20) * (20 / (exp(5)));

                } else {
                    currentState = PAUSE;
                }
                if (currentState == RANDOM_CENTS || secondaryState == RANDOM_CENTS) {
                    for (int i = 0; i < 5; i++) {
                        notes[i]->m_nCents = rand() % 10 - 5;
                    }

                }
                if (currentState == TUBE || secondaryState == TUBE) {
                    AiTube = (AILFOOut1 * 1);
                }

            }
            if(AILFO1.checkWrapModulo()) {
                switch(currentState) {
                    case RANDOM_CENTS:
                    case RANDOM_EIGHTH:
                    case RANDOM_FIFTH:
                        randomSparkle = rand() % 4;
                        break;

                    case TUBE:
                        filter->m_dQControl=2;
                        filter2->m_dQControl=2;
                        randomNoiseR=rand()%100/1000;
                        randomNoiseL=rand()%100/1000;
                        break;

                    case FILTER_SWEEP:
                    case LFO_FILTER:
                        randomNoiseR=rand()%100/1000;
                        randomNoiseL=rand()%100/1000;
                        AILFO1.oscfc=rand()%100/100;
                }


            }

            if( stateTimer++>6*sampleRate) {
                stateTimer=0;
            }


            AiFilter = 0;
            AiLfo = 0;
            AIEnvelopeOut1 = AIEnvelope1.doEnvelope();
            AIEnvelopeOut2 = AIEnvelope2.doEnvelope();
            AIEnvelopeOut3 = AIEnvelope3.doEnvelope();
            AIEnvelopeOut4 = AIEnvelope4.doEnvelope();
            AIEnvelope1.update();
            AIEnvelope2.update();
            AIEnvelope3.update();
            AIEnvelope4.update();

            filtEnvelope = filterEnvelope.doEnvelope();
            filterEnvelope.update();



            AILFOOut1 = AILFO1.doOscillate();
            AILFO1.update();
            AILFOOut2 = AILFO2.doOscillate();
            AILFO2.update();
            AILFOOut3 = AILFO3.doOscillate();
            AILFO3.update();






            // __android_log_print(ANDROID_LOG_DEBUG, "Envelope", "env%f", AIEnvelopeOut1);



            outl = 0, outr = 0;

            // LOGE ("test0");
            if (oscillator_->m_bNoteOn) {


                en = (float) env->doEnvelope();
                lfo = lfoc->doOscillate();


                if (noise) { combinedosc = doWhiteNoise(); }
                else {
                    if (!fmsynth) {

                        oscilate = oscillator_->doOscillate();
                        oscilate0 = oscillator0_->doOscillate();


                        oscilate1 = oscillator1_->doOscillate();
                        oscilate2 = oscillator2_->doOscillate();

                        if (currentState == NOISE_MIX || secondaryState == NOISE_MIX)
                            oscilate2 = doWhiteNoise();
                        combinedosc =
                                ((oscilate0 * 0.3 * AIEnvelopeOut3 +
                                  oscilate1 * 0.2 * AIEnvelopeOut2 +
                                  (oscilate * 0.25 * AIEnvelopeOut1 +
                                   oscilate2 * 0.25 * AIEnvelopeOut4)));
                        oscillator_->setFoModExp(lfo * lfoc->oscfc);
                        oscillator0_->setFoModExp(lfo * lfoc->oscfc);
                        oscillator1_->setFoModExp(lfo * lfoc->oscfc);
                        oscillator2_->setFoModExp(lfo * lfoc->oscfc);
                        oscillator_->update();
                        oscillator0_->update();
                        oscillator1_->update();
                        oscillator2_->update();
                        combinedosc=softClipWaveShaper(combinedosc,randomSparkle+1 );

                    } else {
                        oscilate = oscillator_->doOscillate();

                        oscillator_->update();

                        oscillator0_->setPhaseMod(oscilate * 0.3);
                        oscilate0 = oscillator0_->doOscillate();
                        oscillator0_->update();
                        oscillator1_->setPhaseMod(oscilate0 * (en) * lfo);
                        oscilate1 = oscillator1_->doOscillate();

                        oscillator1_->update();


                        oscillator2_->update();

                        oscilate2 = oscillator2_->doOscillate();
                        oscillator2_->setPhaseMod(oscilate2 * 0.2);
                        oscillator3_->setPhaseMod(oscilate2 * 0.5);
                        oscilate3 = oscillator3_->doOscillate();
                        oscillator3_->update();


                        combinedosc = (oscilate * 0.2 * AIEnvelopeOut3 * AIEnvelopeOut1 +
                                       oscilate1 * 0.3 * AIEnvelopeOut2 + oscilate3 * 0.5);
                        oscillator_->setFoModExp(lfo * lfoc->oscfc);
                        oscillator0_->setFoModExp(lfo * lfoc->oscfc);
                        oscillator1_->setFoModExp(lfo * lfoc->oscfc);
                        oscillator2_->setFoModExp(lfo * lfoc->oscfc);
                        oscillator3_->setFoModExp(lfo * lfoc->oscfc);
                        oscillator3_->update();
                        combinedosc =softClipWaveShaper(combinedosc,randomSparkle+1);

                    }
                }

                env->update();
                lfoc->update();
                if (lfoc->checkWrapModulo()) {
                    AIEnvelope1.reset();
                    AIEnvelope2.reset();
                    if (randomlfo || currentState == LFO_SPEED) {
                        lfoc->m_dOscFo = AiLfo;

                    }
                    if (randomovertones || currentState == OVERTONES) {
                        oscillator1_->m_nOctave = random() % 3 - 1;
                        oscillator_->m_nOctave = random() % 3 - 1;
                    }

                }


                if (currentlfoslow++ == lfoslow * 3) {
                    if (currentState == FILTER_SWEEP || secondaryState == FILTER_SWEEP ||
                        trinaryState == FILTER_SWEEP) {
                        AiFilter = (AILFOOut1 * (AILFOOut2 * AI_LFO_MODIFY + 1)) * 50 * filtEnvelope;


                    }
                    if (currentState == Q1 || secondaryState == Q1 || trinaryState == Q1) {

                        AiQ1 = ((AILFOOut3 + 1) / 2) * 8 + 1;
                        AiQ2 = AiQ1 * 0.70;
                        filter->m_dQControl = limitQ(AiQ1);
                        filter2->m_dQControl = limitQ(AiQ2);
                    }
                    filter->setFcMod(lfoc->filterFc * lfo + AiFilter * AIEnvelopeOut1);
                    filter->update();
                    currentlfoslow = 0;
                    if (currentState == FILTER_SWEEP || secondaryState == FILTER_SWEEP ||
                        trinaryState == FILTER_SWEEP) {
                        AiFilter = (AILFOOut1 * (AILFOOut2 * AI_LFO_MODIFY + 1)) * 50 * filtEnvelope;


                    }

                }
                if (currentlfoslow == lfoslow1) {

                    if (currentState == Q2 || secondaryState == Q2 || trinaryState == Q2) {
                        AiQ2 = ((AILFOOut3 + 1) / 2) * 8 + 1;
                        AiQ1 = AiQ2 * 0.75;
                        filter->m_dQControl = limitQ(AiQ1);
                        filter2->m_dQControl = limitQ(AiQ2);
                    }

                    filter2->setFcMod(lfoc->filterFc * lfo + AiFilter * AIEnvelopeOut2);
                    filter2->update();

                    //  __android_log_print(ANDROID_LOG_ERROR, "tagtest","filter:%s","test");
                }



//                filter->setFcMod(AIEnvelopeOut1);
//                filter2->setFcMod( AIEnvelopeOut1);
                filt = (filter->doFilter(en * combinedosc+randomNoiseL));
                filt2 = (filter2->doFilter(en * combinedosc+randomNoiseR));

                if (tubeon) {
                    filt = softClipWaveShaper(
                            softClipWaveShaper(filt+randomNoiseL, 1 / 80 + 1 + (AiTube)),
                            1/ 90 + 1.3 + AiTube );

                    filt2 = softClipWaveShaper(
                            softClipWaveShaper(filt2+randomNoiseR, 1 / 80 + 1 - AiTube),
                            1 / 90 + 1.3 - AiTube );
                }




                //  outl = filt;

                //' __android_log_print(ANDROID_LOG_ERROR, "tagtest","data:%f",amt);
                if (env->getState() == 0) {
                    oscillator_->stopOscillator();
                    oscillator0_->stopOscillator();
                    oscillator1_->stopOscillator();
                    oscillator2_->stopOscillator();
                    oscillator3_->stopOscillator();
                    env->stopEG();
                }
            } else {
                filt = 0;
                filt2 = 0;
            }

//            if(delaychanged<44100) {}
//                delayChanged++;
//                delay->setWetMix(delayEnvelope.doEnvelope());

            delay->processAudio(&filt, &filt2, &outl, &outr);
            reverb.doReverb(outl, outr, outl, outr);


            if (channels > 1) {
                data[i * channels] = (float) (outl);
                data[i * channels + 1] = (float) (outr);
            } else if (channels == 1) {
                data[i] = (float) outl;
            }


        }
    }

    double filterfc = 0;

    bool AI = false;
    double feedback = 3;

    double filt = 0;
    double filt2 = 0;

    int currentlfoslow = 0;
    double oscilate = 0;
    double oscilate0 = 0;
    double oscilate1 = 0;

    double oscilate2 = 0;
    double oscilate3 = 0;
    double combinedosc=0;

    double randomSpark=2;
    double outl = 0, outr = 0;
    void render(float *data, int32_t numframes) {
        if (AI) {
            performAI(data, numframes);
        } else {

            filt = 0;
            filt2 = 0;

            currentlfoslow = 0;
            oscilate = 0;
            oscilate0 = 0;
            oscilate1 = 0;

            oscilate2 = 0;
            oscilate3 = 0;
            combinedosc=0;


            outl = 0, outr = 0;
            for (int i = 0; i < numframes; i++) {


                outl = 0, outr = 0;


                if (oscillator_->m_bNoteOn) {
                    float en;
                    float lfo;

                    en = (float) env->doEnvelope();
                    lfo = lfoc->doOscillate();

                    if (noise) { combinedosc = doWhiteNoise(); }
                    else {
                        if (!fmsynth) {

                            oscilate = oscillator_->doOscillate();
                            oscilate0 = oscillator0_->doOscillate();


                            oscilate1 = oscillator1_->doOscillate();
                            oscilate2 = oscillator2_->doOscillate();

                            combinedosc =
                                    ((oscilate0 * 0.3 + oscilate1 * 0.2 +
                                      (oscilate * 0.25 + oscilate2 * 0.25)));
                            oscillator_->setFoModExp(lfo * lfoc->oscfc);
                            oscillator0_->setFoModExp(lfo * lfoc->oscfc);
                            oscillator1_->setFoModExp(lfo * lfoc->oscfc);
                            oscillator2_->setFoModExp(lfo * lfoc->oscfc);
                            oscillator_->update();
                            oscillator0_->update();
                            oscillator1_->update();
                            oscillator2_->update();
                            combinedosc=softClipWaveShaper(combinedosc,randomSparkle+1 + (oscillator_->m_uWaveform==4)? 2:0);


                        } else {
                            oscilate = oscillator_->doOscillate();

                            oscillator_->update();

                            oscillator0_->setPhaseMod(oscilate * 0.3);
                            oscilate0 = oscillator0_->doOscillate();
                            oscillator0_->update();
                            oscillator1_->setPhaseMod(oscilate0 * (en) * lfo);
                            oscilate1 = oscillator1_->doOscillate();

                            oscillator1_->update();


                            oscillator2_->update();

                            oscilate2 = oscillator2_->doOscillate();
                            oscillator2_->setPhaseMod(oscilate2 * 0.2);
                            oscillator3_->setPhaseMod(oscilate2 * 0.5);
                            oscilate3 = oscillator3_->doOscillate();
                            oscillator3_->update();


                            combinedosc = (oscilate * 0.2 + oscilate1 * 0.3 + oscilate3 * 0.5);
                            oscillator_->setFoModExp(lfo * lfoc->oscfc);
                            oscillator0_->setFoModExp(lfo * lfoc->oscfc);
                            oscillator1_->setFoModExp(lfo * lfoc->oscfc);
                            oscillator2_->setFoModExp(lfo * lfoc->oscfc);
                            oscillator3_->setFoModExp(lfo * lfoc->oscfc);
                            oscillator3_->update();
                            combinedosc==softClipWaveShaper(combinedosc,randomSparkle+1);

                        }
                    }

                    env->update();
                    lfoc->update();
                    if (lfoc->checkWrapModulo()) {
                        //  __android_log_print(ANDROID_LOG_ERROR, "tagtest","-1 %f",rand());

                        if (randomlfo) {
                            lfoc->m_dOscFo =
                                    exp((double) ((rand() + 30) % 550 / 10) / 20) * (20 / (exp(5)));

                        }
                        if (randomovertones) {
                            oscillator1_->m_nOctave = random() % 3 - 1;
                            oscillator_->m_nOctave = random() % 3 - 1;
                        }

                    }



                    if (currentlfoslow++ == lfoslow) {
                        filter->setFcMod(lfoc->filterFc * lfo);
                        filter->update();
                        currentlfoslow = 0;

                    } else if (currentlfoslow == lfoslow1) {
                        filter2->setFcMod(lfoc->filterFc * lfo);
                        filter2->update();
                        //  __android_log_print(ANDROID_LOG_ERROR, "tagtest","filter:%s","test");
                    }
                    //  __android_log_print(ANDROID_LOG_ERROR, "lfo","lfo %f",lfo);


                    filt = (filter->doFilter(en * combinedosc));
                    filt2 = (filter2->doFilter(en * combinedosc));

                    if (tubeon) {
                        filt = softClipWaveShaper(
                                softClipWaveShaper(filt, 1 / 80 + 1),
                                1 / 90 + 1.3 + (oscillator_->m_uWaveform==4)? 2:0);

                        filt2 = softClipWaveShaper(
                                softClipWaveShaper(filt2, 1 / 80 + 1),
                                1 / 90 + 1.3+ (oscillator_->m_uWaveform==4)? 2:0);
                    }




                    //  outl = filt;

                    //' __android_log_print(ANDROID_LOG_ERROR, "tagtest","data:%f",amt);
                    if (env->getState() == 0) {
                        oscillator_->stopOscillator();
                        oscillator0_->stopOscillator();
                        oscillator1_->stopOscillator();
                        oscillator2_->stopOscillator();
                        oscillator3_->stopOscillator();
                        env->stopEG();
                    }
                } else {
                    filt = 0;
                    filt2 = 0;
                }

                outl=filt;outr=filt2;

                delay->processAudio(&filt, &filt2, &outl, &outr);

//            else {
//                outl=filt;
//                outr=filt;
//            }
                reverb.doReverb(outl, outr, outl, outr);
                // output=outl;

                if (channels > 1) {
                    data[i * channels] = (float) (outl);
                    data[i * channels + 1] = (float) (outr);
                } else if (channels == 1) {
                    data[i] = (float) outl;
                }


            }
        }


    }

    bool delaychanged = false;
    int delaydelay = 10000;

    inline double sgn(double xn) {
        return (xn > 0) - (xn < 0);
    }

    inline double atanWaveShaper(double xn, double saturation) {
        return atan(saturation * xn) / atan(saturation);
    }

    inline double softClipWaveShaper(double xn, double saturation) {
        // --- un-normalized soft clipper from Reiss book

        return sgn(xn) * (1.0 - exp(-fabs(saturation * xn)));
    }

    bool tubeon = true;
    bool looper = false;


    int sampleRate;
    double randomSparkle;
    double randomNoiseL;
    double randomNoiseR;
};