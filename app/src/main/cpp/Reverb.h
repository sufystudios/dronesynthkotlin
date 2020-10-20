//
// Created by sufy on 25/05/20.
//

#ifndef DRONESYNTHGOOD_MASTER_REVERB_H
#define DRONESYNTHGOOD_MASTER_REVERB_H


#include "Delay.h"
#include "OnePoleLPF.h"
#include "DelayAPF.h"
#include "CombFilter.h"

class Reverb {
public:
    ~Reverb() {

    }
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

    void prepareReverb(int m_nSampleRate) {
        m_PreDelay.init((int)(0.1*(m_nSampleRate)));

        // init up to 100 mSec
        m_InputAPF_1.init((int)(0.1*(m_nSampleRate)));
        m_InputAPF_2.init((int)(0.1*(m_nSampleRate)));

        // 100 mSec each max
        m_ParallelCF_1.init((int)(0.1*m_nSampleRate));
        m_ParallelCF_2.init((int)(0.1*m_nSampleRate));
        m_ParallelCF_3.init((int)(0.1*m_nSampleRate));
        m_ParallelCF_4.init((int)(0.1*m_nSampleRate));
        m_ParallelCF_5.init((int)(0.1*m_nSampleRate));
        m_ParallelCF_6.init((int)(0.1*m_nSampleRate));
        m_ParallelCF_7.init((int)(0.1*m_nSampleRate));
        m_ParallelCF_8.init((int)(0.1*m_nSampleRate));

        // 100 mSec each max
        m_OutputAPF_3.init((int)(0.1*m_nSampleRate));
        m_OutputAPF_4.init((int)(0.1*(m_nSampleRate)));

        // init the three LPFs
        m_InputLPF.init();
        m_DampingLPF1.init();
        m_DampingLPF2.init();

        // Call all delay resets
        m_PreDelay.resetDelay();
        m_InputAPF_1.resetDelay();
        m_InputAPF_2.resetDelay();

        m_ParallelCF_1.resetDelay();
        m_ParallelCF_2.resetDelay();
        m_ParallelCF_3.resetDelay();
        m_ParallelCF_4.resetDelay();
        m_ParallelCF_5.resetDelay();
        m_ParallelCF_6.resetDelay();
        m_ParallelCF_7.resetDelay();
        m_ParallelCF_8.resetDelay();

        m_OutputAPF_3.resetDelay();
        m_OutputAPF_4.resetDelay();

        // set sample rates on combs (needed to calc g values)
        m_ParallelCF_1.setSampleRate(m_nSampleRate);
        m_ParallelCF_2.setSampleRate(m_nSampleRate);
        m_ParallelCF_3.setSampleRate(m_nSampleRate);
        m_ParallelCF_4.setSampleRate(m_nSampleRate);
        m_ParallelCF_5.setSampleRate(m_nSampleRate);
        m_ParallelCF_6.setSampleRate(m_nSampleRate);
        m_ParallelCF_7.setSampleRate(m_nSampleRate);
        m_ParallelCF_8.setSampleRate(m_nSampleRate);

        // flush buffers
        m_InputLPF.init();
        m_DampingLPF1.init();
        m_DampingLPF2.init();

        // cook everything
        cookVariables();

    }
    float in1f;
    float in2f;
     float fPC_1_Out = 0;
    float fPC_2_Out = 0;
    float fPC_3_Out = 0;
    float fPC_4_Out = 0;
    float fPC_5_Out = 0;
    float fPC_6_Out = 0;
    float fPC_7_Out = 0;
    float fPC_8_Out = 0;
    float fC1_Out = 0;
    float fC2_Out = 0;
    void doReverb(double &in1,double &in2, double &out1, double &out2) {
        float fPreDelayOut = 0;
        //  __android_log_print(ANDROID_LOG_ERROR, "tagtest","note : %d  : Freq");
     //  m_PreDelay.processAudio(&in, &fPreDelayOut);
        // __android_log_print(ANDROID_LOG_ERROR, "tagtest","note : %d  : Freq");
        // Pre-Delay Out -> fAPF_1_Out
    //    float fAPF_1_Out = 0;
       // m_InputAPF_1.processAudio(&fPreDelayOut, &fAPF_1_Out);

        // fAPF_1_Out -> fAPF_2_Out
        float fAPF_2_Out = 0;
        in1f=(float)in1;

        in2f=(float)in2;

        m_InputAPF_1.processAudio(&in1f,&in1f);
        m_InputAPF_2.processAudio(&in2f,&in2f);

      //  m_InputLPF.processAudio(&fAPF_2_Out, &fInputLPF);

        // comb filter bank
        // variables for each output
        fPC_1_Out = 0;
        fPC_2_Out = 0;
        fPC_3_Out = 0;
        fPC_4_Out = 0;
        fPC_5_Out = 0;
        fPC_6_Out = 0;
        fPC_7_Out = 0;
        fPC_8_Out = 0;
        fC1_Out = 0;
        fC2_Out = 0;



        // fInputLPF -> fPC_1_Out, fPC_2_Out, fPC_3_Out, fPC_4_Out
        m_ParallelCF_1.processAudio(&in1f, &fPC_1_Out);
        m_ParallelCF_2.processAudio(&in1f, &fPC_2_Out);
        m_ParallelCF_3.processAudio(&in1f, &fPC_3_Out);
        m_ParallelCF_4.processAudio(&in1f, &fPC_4_Out);

        // fInputLPF -> fPC_5_Out, fPC_6_Out, fPC_7_Out, fPC_8_Out
        m_ParallelCF_5.processAudio(&in2f, &fPC_5_Out);
        m_ParallelCF_6.processAudio(&in2f, &fPC_6_Out);
        m_ParallelCF_7.processAudio(&in2f, &fPC_7_Out);
        m_ParallelCF_8.processAudio(&in2f, &fPC_8_Out);

        // form outputs: note attenuation by 0.25 for each and alternating signs
        fC1_Out = 0.25*fPC_1_Out -  0.25*fPC_2_Out + 0.25*fPC_3_Out - 0.25*fPC_4_Out;
  fC2_Out = 0.25*fPC_5_Out -  0.25*fPC_6_Out + 0.25*fPC_7_Out - 0.25*fPC_8_Out;
  fC1_Out=fC1_Out*0.9+fC2_Out*0.1;
  fC2_Out=fC2_Out*0.9+fC1_Out*0.1;

        // fC1_Out -> fDamping_LPF_1_Out
        float fDamping_LPF_1_Out = 0;
        m_DampingLPF1.processAudio(&fC1_Out, &fDamping_LPF_1_Out);

        // fC2_Out -> fDamping_LPF_2_Out
        float fDamping_LPF_2_Out = 0;
        m_DampingLPF2.processAudio(&fC2_Out, &fDamping_LPF_2_Out);

//        // fDamping_LPF_1_Out -> fAPF_3_Out
//        float fAPF_3_Out = 0;
//        m_OutputAPF_3.processAudio(&fDamping_LPF_1_Out, &fAPF_3_Out);

//        // fDamping_LPF_2_Out -> fAPF_4_Out
//        float fAPF_4_Out = 0;
//        m_OutputAPF_4.processAudio(&fDamping_LPF_2_Out, &fAPF_4_Out);

        // form output = (100-Wet)/100*x(n) + (Wet/100)*fAPF_3_Out
     out1=(double)((100.0 - m_fWet_pct)/100.0)*in1 +
                    (m_fWet_pct/100.0)*(fDamping_LPF_1_Out);
out2=(double)((100.0 - m_fWet_pct)/100.0)*in2 +
     (m_fWet_pct/100.0)*(fDamping_LPF_2_Out);
        // Do RIGHT Channel if there is one
        //return out;

    }
    void cookVariables()
    {
        // Pre-Delay
        m_PreDelay.setDelay_mSec(6);
        m_PreDelay.setOutputAttenuation_dB(0);

        // input diffusion
        m_InputAPF_1.setDelay_mSec(24);
        m_InputAPF_1.setAPF_g(0.7);

        m_InputAPF_2.setDelay_mSec(23);
        m_InputAPF_2.setAPF_g(0.6);

//        // output diffusion_with_RTSixty
//        m_OutputAPF_3.setDelay_mSec(64);
//        m_OutputAPF_3.setAPF_g(0);
//
//        m_OutputAPF_4.setDelay_mSec(86);
//        m_OutputAPF_4.setAPF_g(0);

        // Comb Filters
        // set delays first...
        m_ParallelCF_1.setDelay_mSec(27);
        m_ParallelCF_2.setDelay_mSec(53);
        m_ParallelCF_3.setDelay_mSec(84);
        m_ParallelCF_4.setDelay_mSec(76);
        m_ParallelCF_5.setDelay_mSec(37);
        m_ParallelCF_6.setDelay_mSec(95);
        m_ParallelCF_7.setDelay_mSec(34);
        m_ParallelCF_8.setDelay_mSec(85);

        // ...then calcualte comb g's from RT60:
        m_ParallelCF_1.setComb_g_with_RTSixty(64);
        m_ParallelCF_2.setComb_g_with_RTSixty(23);
        m_ParallelCF_3.setComb_g_with_RTSixty(54);
        m_ParallelCF_4.setComb_g_with_RTSixty(29);
        m_ParallelCF_5.setComb_g_with_RTSixty(43);
        m_ParallelCF_6.setComb_g_with_RTSixty(95);
        m_ParallelCF_7.setComb_g_with_RTSixty(32);
        m_ParallelCF_8.setComb_g_with_RTSixty(29);
        // LPFs_with_RTSixty
        m_DampingLPF1.setLPF_g(0.5);
        m_DampingLPF2.setLPF_g(0.5);

    }



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
};
#endif //DRONESYNTHGOOD_MASTER_REVERB_H
