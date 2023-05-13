#include "ModulatedDelay.h"

class Flanger {

public:
    CModulatedDelay m_ModDelayLeft;
    CModulatedDelay m_ModDelayRight;

    void updateModDelays();

    // END OF USER CODE -------------------------------------------------------------- //


    // ADDED BY RACKAFX -- DO NOT EDIT THIS CODE!!! ----------------------------------- //
    //  **--0x07FD--**

    float m_fModDepth_pct;
    float m_fModFrequency_Hz;
    float m_fFeedback_pct;
    UINT m_uLFOType;
    enum{tri,sine};


    bool processAudio(double* pInputBuffer, double* pOutputBuffer, double* pInputBuffer0, double* pOutputBuffer0) {

        {

            // Stereo-In, Stereo-Out (INSERT Effect)
          m_ModDelayLeft.processAudioFrame(pInputBuffer,pOutputBuffer,2,2);
          m_ModDelayRight.processAudioFrame(pInputBuffer0,pOutputBuffer0,2,2);

            return true;
        }


}}



;