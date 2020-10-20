//
// Created by sufy on 17/12/2018.
//

#include <cstdint>
#include <android/log.h>
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

#ifndef WAVEMAKER_MUSICSYSTEM_H
#define WAVEMAKER_MUSICSYSTEM_H

#endif //WAVEMAKER_MUSICSYSTEM_H
class MusicSystem {

public :
#define AMPLITUDE 0.3
    CWTOscillator* oscillator_;
    CWTOscillator* oscillator0_;
    CWTOscillator* oscillator1_;
    CWTOscillator* oscillator2_;
    CWTOscillator* oscillator3_;

    Reverb* stereoReverb;

    void setReverb(Reverb* rev) {
        stereoReverb = rev;
    }

    CEnvelopeGenerator* env;
    bool fmsynth=false;
    CStereoDelayFX* delay;
    CModulationMatrix* globalmm;
    CDCA* dca;
    CMoogLadderFilter* filter;
    CLFO* lfoc;
    int channels = 2;

    void setChannels(int c) {
        channels = c;
    }
//REVERB
    CDelay m_PreDelay;

    // input Diffusion
    COnePoleLPF m_InputLPF;
    CDelayAPF m_InputAPF_1;
    CDelayAPF m_InputAPF_2;

    // parallel Comb Bank 1
    CCombFilter m_ParallelCF_1;
    CCombFilter m_ParallelCF_2;
    CCombFilter m_ParallelCF_3;
    CCombFilter m_ParallelCF_4;

    // parallel Comb Bank 2
    CCombFilter m_ParallelCF_5;
    CCombFilter m_ParallelCF_6;
    CCombFilter m_ParallelCF_7;
    CCombFilter m_ParallelCF_8;

    // damping
    COnePoleLPF m_DampingLPF1;
    COnePoleLPF m_DampingLPF2;

    // output diffusion
    CDelayAPF m_OutputAPF_3;
    CDelayAPF m_OutputAPF_4;

    // function to cook all member object's variables at once


    float m_fPreDelay_mSec;
    float m_fPreDelayAtten_dB;
    float m_fInputLPF_g;
    float m_fAPF_1_Delay_mSec;
    float m_fAPF_1_g;
    float m_fAPF_2_Delay_mSec;
    float m_fAPF_2_g;
    float m_fRT60;
    float m_fWet_pct;
    float m_fPComb_1_Delay_mSec;
    float m_fPComb_2_Delay_mSec;
    float m_fPComb_3_Delay_mSec;
    float m_fPComb_4_Delay_mSec;
    float m_fLPF2_g2;
    float m_fAPF_3_Delay_mSec;
    float m_fAPF_3_g;
    float m_fPComb_5_Delay_mSec;
    float m_fPComb_6_Delay_mSec;
    float m_fPComb_7_Delay_mSec;
    float m_fPComb_8_Delay_mSec;
    float m_fAPF_4_Delay_mSec;
    float m_fAPF_4_g;
    void setOsc(CWTOscillator* b, CWTOscillator* c, CWTOscillator* d, CWTOscillator* e, CWTOscillator* f) {
        oscillator_=b;
        oscillator0_=c;
        oscillator1_=d;
        oscillator2_=e;
        oscillator3_=f;
    }
    void setFilter(CMoogLadderFilter* fil) {
        filter=fil;
    }
    void setModMatrix(CModulationMatrix* global) {
        globalmm=global;
    }
    void setEnv(CEnvelopeGenerator* enve) {
        env=enve;
    }
    void setDelay(CStereoDelayFX* del) {
        delay=del;
    }
    void setLFO(CLFO* lfo) {
        lfoc=lfo;
    }
    void setDCA(CDCA* dcal) {
        dca=dcal;
    }

    double filterfc = 0;
    void render(float *data, int32_t numframes) {

        bool reverb=true;
        double fmval=4;
        double output = 0;
        double filt = 0;
        int currentlfoslow=0;
        int lfoslow=4;

        for(int i=0;i<numframes;i++) {

            double oscilate = 0;
            double oscilate0 = 0;
            double oscilate1 = 0;
            double oscilate2=0;
            double oscilate3=0;
            double outl=0,outr=0;



            // LOGE ("test0");
            if (oscillator_->m_bNoteOn) {
//LOGE ("test");

                float en = (float) env->doEnvelope();
                float lfo=lfoc->doOscillate();
                //   LOGE ("test1");
                // dca->setEGMod(en * 1);
                // dca->update();
                double feedback=3;
                double combinedosc;
                if(!fmsynth) {
                    oscilate = oscillator_->doOscillate();
                    oscilate0=oscillator0_->doOscillate();


                    oscilate1=oscillator1_->doOscillate();
                    oscilate2=oscillator2_->doOscillate();

                    combinedosc = en * (oscilate0*0.3+oscilate1*0.1 +oscilate*0.3+oscilate2*0.3);
                    oscillator_->setFoModExp(lfo * lfoc->oscfc);
                    oscillator0_->setFoModExp(lfo * lfoc->oscfc);
                    oscillator1_->setFoModExp(lfo * lfoc->oscfc);
                    oscillator2_->setFoModExp(lfo*lfoc->oscfc);
                    oscillator_->update();
                    oscillator0_->update();
                    oscillator1_->update();
                    oscillator2_->update();
                    if(oscillator_->m_uWaveform==4)
                       combinedosc= softClipWaveShaper(combinedosc,2);


                }else {
                    oscilate=oscillator_->doOscillate();
                    oscillator_->setPhaseMod(oscilate*(feedback/7)*0.7);
                    oscillator_->update();
                    oscillator0_->setPhaseMod(oscilate*0.7*0.5);
                    oscillator0_->update();
                    oscilate0=oscillator0_->doOscillate();
                    oscilate1=oscillator1_->doOscillate();

                    oscillator1_->update();

                    oscillator2_->setPhaseMod(oscilate1*0.7);
                    oscillator2_->update();
                    oscilate2=oscillator2_->doOscillate();
                    combinedosc = en * (oscilate *0.13+oscilate2*0.34+oscilate0*0.14+ oscillator_->doOscillate()*0.40);
                    oscillator_->setFoModExp(lfo * lfoc->oscfc );
                    oscillator0_->setFoModExp(lfo * lfoc->oscfc );
                    oscillator1_->setFoModExp(lfo*lfoc->oscfc);
                    oscillator2_->setFoModExp(lfo * lfoc->oscfc);



                }
                double filtin = 0;
                //dca->doDCA(combinedosc,combinedosc,filtin,filtin);



                //  oscilate*0.33+oscilate0*0.33+oscilate1*0.33;
                env->update();
                lfoc->update();



//                if(fmsynth==false) {
//                    oscillator_->setFoModExp(lfo * lfoc->oscfc);
//                    oscillator0_->setFoModExp(lfo * lfoc->oscfc);
//                    oscillator1_->setFoModExp(lfo * lfoc->oscfc);
//                } else {
//                    oscillator_->setFoModExp(oscilate1*lfoc->oscfc);
//                    oscillator0_->setFoModExp(oscilate1*lfoc->oscfc);
//
//                }
                //filter->setFcMod(lfo * lfoc->filterFc);
                // globalmm->doModulationMatrix(1);
                if(currentlfoslow==lfoslow) {
                    filter->setFcMod(lfoc->filterFc*lfo);
                    filter->update();
                    currentlfoslow=0;
                }else {
                    currentlfoslow++;
                }
                //  __android_log_print(ANDROID_LOG_ERROR, "lfo","lfo %f",lfo);

                // filt = filter->doFilter(combinedosc);
//if(1!=1)
                // filt=filter->doFilter(1.2*softClipWaveShaper(softClipWaveShaper(softClipWaveShaper(combinedosc,0.7),0.8),0.9));

                //else
                filt=filter->doFilter(combinedosc);
                if (tubeon) {
                    filt = softClipWaveShaper(
                            softClipWaveShaper(filt, stereoReverb->m_fWet_pct / 80 + 1),
                            stereoReverb->m_fWet_pct / 90 + 1.3 + (oscillator_->m_uWaveform==4)? 2:0);


                }
                // __android_log_print(ANDROID_LOG_ERROR, "tagtest","filter:%f",filt);
                if(filt>1)
                    filt=1;
                else if (filt<-1)
                    filt=-1;
                //  outl = filt;

                //' __android_log_print(ANDROID_LOG_ERROR, "tagtest","data:%f",amt);
                if (env->getState() == 0) {
                    oscillator_->stopOscillator();
                    oscillator0_->stopOscillator();
                    oscillator1_->stopOscillator();
                    oscillator2_->stopOscillator();

                    env->stopEG();
                }
      stereoReverb->doReverb(filt, filt, outl , outr);

            }





            delay->processAudio(&outl, &outr, &outl, &outr);
            // delay->update();

            if (channels > 1) {
                data[i * channels] = (float) (outl);
                data[i * channels + 1] = (float) (outr);
            } else if (channels == 1) {
                data[i] = (float) outl;
            }

        }

    }

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

    bool tubeon=true;

};