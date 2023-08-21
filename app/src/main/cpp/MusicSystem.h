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
//#include "Flanger.h"
//#include "OboeAudioRecorder.h"

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
    //Flanger flanger;
    double distortion = 0.5;
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
//        filterpurchase2->m_dSaturation=2;

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
        //FILTER_SWEEP,
        //Q1,
        //Q2,
        //DELAY,
        //OVERTONES,
        //LFO_FILTER,
        //LFO_SPEED,
        //TUBE,
        //NOISE_MIX,
        //RANDOM_CENTS,
       // RANDOM_FIFTH,
        //SUB,
        //RANDOM_EIGHTH
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


    double filterfc = 0;

    bool filterpurchase2 = false;
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
        if (false) {
            //performAI(data, numframes);
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
                           // oscilate0 = oscillator0_->doOscillate();


                            //oscilate1 = oscillator1_->doOscillate();
                            //oscilate2 = oscillator2_->doOscillate();
combinedosc=oscilate*0.5+oscilate0*0.5;
                           /*combinedosc =
                                    ((oscilate0 * 0.3 + oscilate1 * 0.2 +
                                      (oscilate * 0.25 + oscilate2 * 0.25)));
                                      */
                            oscillator_->setFoModExp(lfo * lfoc->oscfc);
                            oscillator0_->setFoModExp(lfo*lfoc->oscfc);
                            //oscillator0_->setFoModExp(lfo * lfoc->oscfc);
                            //oscillator1_->setFoModExp(lfo * lfoc->oscfc);
                            //oscillator2_->setFoModExp(lfo * lfoc->oscfc);
                            oscillator_->update();
                            oscillator0_->update();
                            //oscillator0_->update();
                            //oscillator1_->update();
                            //oscillator2_->update();
                            //combinedosc=softClipWaveShaper(combinedosc,randomSparkle+1 + (oscillator_->m_uWaveform==4)? 2:0);


                        } else {
                            oscilate = oscillator_->doOscillate();

                            oscillator_->update();


                            oscilate0 = oscillator0_->doOscillate();
                            oscillator0_->setPhaseMod(oscilate0 * 0.1);
                            oscillator0_->setPhaseMod(oscilate*0.1);
                            oscillator0_->update();
                           // oscillator1_->setPhaseMod(oscilate0 * (en) * lfo);
                            //oscilate1 = oscillator1_->doOscillate();

                            //oscillator1_->update();


                           // oscillator2_->update();

                          //  oscilate2 = oscillator2_->doOscillate();
                          //  oscillator2_->setPhaseMod(oscilate2 * 0.2);
                           // oscillator3_->setPhaseMod(oscilate2 * 0.5);
                           // oscilate3 = oscillator3_->doOscillate();
                           // oscillator3_->update();

                            combinedosc=oscilate*0.5+oscilate0*0.5;
                           // combinedosc = (oscilate * 0.2 + oscilate1 * 0.3 + oscilate3 * 0.5);
                            oscillator_->setFoModExp(lfo * lfoc->oscfc);
                            oscillator0_->setFoModExp(lfo * lfoc->oscfc);
                           /* oscillator1_->setFoModExp(lfo * lfoc->oscfc);
                            oscillator2_->setFoModExp(lfo * lfoc->oscfc);
                            oscillator3_->setFoModExp(lfo * lfoc->oscfc);
                            oscillator3_->update();
                            */
                            combinedosc==softClipWaveShaper(combinedosc,randomSparkle+1);

                        }
                    }

                    env->update();
                    lfoc->update();




                   // if (currentlfoslow++ == lfoslow) {
                        filter->setFcMod(lfoc->filterFc * lfo);
                        filter->update();
                        filter2->setFcMod(lfoc->filterFc*lfo);
                        filter2->update();
                       // currentlfoslow = 0;

                    //} else if (currentlfoslow == lfoslow1) {
                      //  filter2->setFcMod(lfoc->filterFc * lfo);
                       // filter2->update();
                        //  __android_log_print(ANDROID_LOG_ERROR, "tagtest","filter:%s","test");
                   // }
                    //  __android_log_print(ANDROID_LOG_ERROR, "lfo","lfo %f",lfo);


                    filt = (filter->doFilter(en * combinedosc));
                            //if(filterpurchase2)
                    filt2 = (filter2->doFilter(en * combinedosc));
                           // else
                               // filt2=filt;

                    if (tubeon) {
                        filt = softClipWaveShaper(
                                filt, distortion);
if(filterpurchase2)
                        filt2 = softClipWaveShaper(
                                filt2, distortion);
                    else
                        filt2=filt;
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

                //outl=filt;outr=filt2;

                delay->processAudio(&filt, &filt2, &outl, &outr);

//            else {
//                outl=filt;
//                outr=filt;
//            }
                reverb.doReverb(outl, outr, outl, outr);
                // output=outl;
                //flanger.processAudio();
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