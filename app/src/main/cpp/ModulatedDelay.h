#include "DelayLine.h"
#include "WTOscillator.h"

class CModulatedDelay  {
public:
    CModulatedDelay();
    float m_fModDepth_pct;
    float m_fModFrequency_Hz;
    float m_fFeedback_pct;
    float m_fChorusOffset;
    UINT m_uModType;
    enum{Flanger,Vibrato,Chorus};
    UINT m_uLFOType;
    enum{tri,sin};
    UINT m_uLFOPhase;
    enum{normal,quad,invert};


    void updateModDelays();

    // END OF USER CODE -------------------------------------------------------------- //


    // ADDED BY RACKAFX -- DO NOT EDIT THIS CODE!!! ----------------------------------- //
    //  **--0x07FD--**

void *numbers;

    CWTOscillator m_LFO;    // our LFO
    CDelayLine m_DDL;    // our delay line module

    // these will depend on the type of mod
    float m_fMinDelay_mSec;
    float m_fMaxDelay_mSec;

    // functions to update the member objects
    void updateLFO();

    void updateDDL();

    // cooking function for mod type
    //void cookModType();

    // convert a LFO value to a delay offset value
    float calculateDelayOffset(float fLFOSample);

    // 2. One Time Destruction
    virtual ~CModulatedDelay(void);

    virtual bool
    __stdcall processAudioFrame(double *pInputBuffer, double *pOutputBuffer, UINT uNumInputChannels,
                                UINT uNumOutputChannels);
};