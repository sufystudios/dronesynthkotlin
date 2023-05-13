#include "ModulatedDelay.h"

CModulatedDelay::CModulatedDelay() {
    numbers = malloc(44100);
};
CModulatedDelay::~CModulatedDelay() {}

void CModulatedDelay::updateLFO() {
    m_LFO.update();

}

bool
CModulatedDelay::processAudioFrame(double *pInputBuffer, double *pOutputBuffer, UINT uNumInputChannels,
                                  UINT uNumOutputChannels) {
    float fYn = 0;
    float fYqn = 0;
    m_LFO.doOscillate();

    // 2. calculate delay offset
    float fDelay = 0.0;
    if(m_uLFOPhase == quad)
        fDelay = calculateDelayOffset(fYqn); // quadrature LFO
    else
        fDelay = calculateDelayOffset(fYn); // normal LFO

    // 3. set the delay & cook
    m_DDL.m_dDelay_ms = fDelay;
    m_DDL.cookVariables();

    // 4. get the delay output one channel in/one channel out
    m_DDL.processAudio(&pInputBuffer[0], &pOutputBuffer[0]);

    // Mono-In, Stereo-Out (AUX Effect)
    if(uNumInputChannels == 1 && uNumOutputChannels == 2)
        pOutputBuffer[1] = pOutputBuffer[0];

    // Stereo-In, Stereo-Out (INSERT Effect)
    if(uNumInputChannels == 2 && uNumOutputChannels == 2)
        pOutputBuffer[1] = pOutputBuffer[0];

    return true;

}

void CModulatedDelay::updateDDL() {
//
}

float CModulatedDelay::calculateDelayOffset(float fLFOSample) {
    if(m_uModType == Flanger || m_uModType == Vibrato)
    {
        // flanger 0->1 gets mapped to 0->maxdelay
        return (m_fModDepth_pct/100.0)*(fLFOSample*(m_fMaxDelay_mSec - m_fMinDelay_mSec)) + m_fMinDelay_mSec;
    }
    else if(m_uModType == Chorus)
    {
        // chorus adds starting offset to move delay range
        float fStart = m_fMinDelay_mSec + m_fChorusOffset;

        return (m_fModDepth_pct/100.0)*(fLFOSample*(m_fMaxDelay_mSec - m_fMinDelay_mSec)) + fStart;
    }

}
