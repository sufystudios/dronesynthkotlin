
#pragma once

#include <math.h>

class CDelay
{
public:
	// constructor/destructor
	CDelay(void);
	virtual ~CDelay(void);  // base class MUST declare virtual destructor

// delay in mSec, set by Parent Plug In
float m_fDelay_ms;
protected:
	// member variables
	//
	// delay in samples; float supports fractional delay
	float m_fDelayInSamples;

	// output attenuation value, cooked
	float m_fOutputAttenuation;

	// pointer to our circular buffer
	float* m_pBuffer;

	// read/write index values for circ buffer
	int m_nReadIndex;
	int m_nWriteIndex;

	// max length of buffer
	int m_nBufferSize;

	// sample rate (needed for other function)
	int m_nSampleRate;

    // Attenuation in dB, set by Parent Plug In
	float m_fOutputAttenuation_dB;

	// functions for the owner plug-in to call
public:
	// declare buffer and zero
	void init(int nDelayLength);

	// function to cook variables
	void cookVariables();

	// flush buffer, reset pointers to top
	void resetDelay();

	// set functions for Parent Plug In
	void setDelay_mSec(float fmSec);
	void setSampleRate(int nFs){m_nSampleRate = nFs;};
	void setOutputAttenuation_dB(float fAttendB);

	// read the delay without writing or incrementing
	float readDelay();

	// read the delay at an arbitrary time without writing or incrementing
	float readDelayAt(float fmSec);

	// write the input and icrement pointers
	void writeDelayAndInc(float fDelayInput);

	// process audio
	bool processAudio(float* pInput, float* pOutput);

};

