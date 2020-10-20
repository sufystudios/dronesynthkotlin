#pragma once
#include "pluginconstants.h"
#include "lookuptables.h"
#include "time.h"
#include <cmath>


#define CONVEX_LIMIT 0.00398107
#define CONCAVE_LIMIT 0.99601893
#define ARC4RANDOMMAX 4294967295 // (2^32 - 1)

#define EXTRACT_BITS(the_val, bits_start, bits_len) ((the_val >> (bits_start - 1)) & ((1 << bits_len) - 1))

inline double pitchShiftMultiplier(double dPitchShiftSemitones)
{
    if(dPitchShiftSemitones == 0)
        return 1.0;

    // 2^(N/12)
//	return fastPow(2.0, dPitchShiftSemitones/12.0);
    return pow(2.0, dPitchShiftSemitones/12.0);
}
inline double convexTransform(double dValue)
{
    if(dValue <= CONVEX_LIMIT)
        return 0.0;

    return 1.0 + (5.0/12.0)*log10(dValue);
}

/* convexInvertedTransform()
	
	calculates the convexInvertedTransform of the input
	
	dValue = the unipolar (0 -> 1) value to convert
*/
inline double doWhiteNoise()
{
    float fNoise = 0.0;

#if defined _WINDOWS || defined _WINDLL    
    // fNoise is 0 -> 32767.0
	fNoise = (float)rand();

	// normalize and make bipolar
	fNoise = 2.0*(fNoise/32767.0) - 1.0;
#else
    // fNoise is 0 -> ARC4RANDOMMAX
    fNoise = (float)arc4random();

    // normalize and make bipolar
    fNoise = 2.0*(fNoise/ARC4RANDOMMAX) - 1.0;
#endif

    return fNoise;
}

inline double doPNSequence(UINT& uPNRegister)
{
    // get the bits
    UINT b0 = EXTRACT_BITS(uPNRegister, 1, 1); // 1 = b0 is FIRST bit from right
    UINT b1 = EXTRACT_BITS(uPNRegister, 2, 1); // 1 = b1 is SECOND bit from right
    UINT b27 = EXTRACT_BITS(uPNRegister, 28, 1); // 28 = b27 is 28th bit from right
    UINT b28 = EXTRACT_BITS(uPNRegister, 29, 1); // 29 = b28 is 29th bit from right

    // form the XOR
    UINT b31 = b0^b1^b27^b28;

    // form the mask to OR with the register to load b31
    if(b31 == 1)
        b31 = 0x10000000;

    // shift one bit to right
    uPNRegister >>= 1;

    // set the b31 bit
    uPNRegister |= b31;

    // convert the output into a floating point number, scaled by experimentation
    // to a range of o to +2.0
    float fOut = (float)(uPNRegister)/((pow((float)2.0,(float)32.0))/16.0);

    // shift down to form a result from -1.0 to +1.0
    fOut -= 1.0;

    return fOut;
}
inline double convexInvertedTransform(double dValue)
{
    if(dValue >= CONCAVE_LIMIT)
        return 0.0;

    return 1.0 + (5.0/12.0)*log10(1.0 - dValue);
}
inline double concaveInvertedTransform(double dValue)
{
    if(dValue <= CONVEX_LIMIT)
        return 1.0;

    return -(5.0/12.0)*log10(dValue);
}
/* concaveTransform()
	
	calculates the concaveTransform of the input
	
	dValue = the unipolar (0 -> 1) value to convert
*/
inline double concaveTransform(double dValue)
{
    if(dValue >= CONCAVE_LIMIT)
        return 1.0;

    return -(5.0/12.0)*log10(1.0 - dValue);
}

const double B = 4.0/(float)pi;
const double C = -4.0/((float)pi*(float)pi);
const double P = 0.225;
inline double parabolicSine(double x, bool bHighPrecision = true)
{
    double y = B * x + C * x * fabs(x);

    if(bHighPrecision)
        y = P * (y * fabs(y) - y) + y;

    return y;
}
static inline float fastlog2 (float x)
{
    union { float f; unsigned int i; } vx = { x };
    union { unsigned int i; float f; } mx = { (vx.i & 0x007FFFFF) | 0x3f000000 };
    float y = vx.i;
    y *= 1.1920928955078125e-7f;

    return y - 124.22551499f
           - 1.498030302f * mx.f
           - 1.72587999f / (0.3520887068f + mx.f);
}
inline void calculatePanValues(double dPanMod, double& dLeftPanValue, double& dRightPanValue)
{
    //dLeftPanValue = fastcos((pi/4.0)*(dPanMod + 1.0));
    //dRightPanValue = fastsin((pi/4.0)*(dPanMod + 1.0));

    dLeftPanValue = cos((pi/4.0)*(dPanMod + 1.0));
    dRightPanValue = sin((pi/4.0)*(dPanMod + 1.0));

    dLeftPanValue = fmax(dLeftPanValue, (double)0.0);
    dLeftPanValue = fmin(dLeftPanValue, (double)1.0);
    dRightPanValue = fmax(dRightPanValue, (double)0.0);
    dRightPanValue = fmin(dRightPanValue, (double)1.0);
}

enum sources{
    SOURCE_NONE = 0,

    SOURCE_UI_OSC1_FO,
    SOURCE_UI_OSC2_FO,
    SOURCE_UI_TO_OSC3_FO,
    SOURCE_UI_TO_OSC4_FO,
    SOURCE_UI_TO_ALL_OSC_FO,

    SOURCE_UI_FILTER1_FC,
    SOURCE_UI_FILTER2_FC,
    SOURCE_UI_ALL_FILTER_FC,
    SOURCE_UI_FILTER1_Q,
    SOURCE_UI_FILTER2_Q,
    SOURCE_UI_ALL_FILTER_Q,

    SOURCE_LFO1,
    SOURCE_LFO2,
    SOURCE_LFO1Q,					/* QuadPhase outputs */
    SOURCE_LFO2Q,
    SOURCE_EG1,						/* Exponential outputs */
    SOURCE_EG2,
    SOURCE_EG3,
    SOURCE_EG4,
    SOURCE_BIASED_EG1,				/* Exponential outputs with Sustain subtracted*/
    SOURCE_BIASED_EG2,
    SOURCE_BIASED_EG3,
    SOURCE_BIASED_EG4,
    SOURCE_VELOCITY,
    SOURCE_MIDI_VOLUME_CC07,		/* MIDI Volume */
    SOURCE_MIDI_PAN_CC10,			/* MIDI Pan */
    SOURCE_MIDI_EXPRESSION_CC11,	/* MIDI Expression */
    SOURCE_MODWHEEL,
    SOURCE_PITCHBEND,
    SOURCE_SUSTAIN_PEDAL,
    SOURCE_MIDI_NOTE_NUM,
    SOURCE_MIDI_JS_X,
    SOURCE_MIDI_JS_Y,
    MAX_SOURCES
};


enum destinations{
    DEST_NONE = 0,

    // --- GUI control layer modulation (-1)
            DEST_UI_OSC1_FO,
    DEST_UI_OSC2_FO,
    DEST_UI_TO_OSC3_FO,
    DEST_UI_TO_OSC4_FO,
    DEST_UI_TO_ALL_OSC_FO,

    DEST_UI_FILTER1_FC,
    DEST_UI_FILTER2_FC,
    DEST_UI_ALL_FILTER_FC,
    DEST_UI_FILTER1_Q,
    DEST_UI_FILTER2_Q,
    DEST_UI_ALL_FILTER_Q,

    // --- LAYER 0 DESTINATIONS
    //     add more L0 destinations in this chunk
    //     see CModulationMatrix::checkDestinationLayer())
            DEST_LFO1_FO, // <- keep this first
    DEST_LFO2_FO,
    DEST_ALL_LFO_FO,
    DEST_LFO1_OUTPUT_AMP,
    DEST_LFO2_OUTPUT_AMP,
    DEST_ALL_LFO_OUTPUT_AMP,
    DEST_EG1_ATTACK_SCALING,
    DEST_EG2_ATTACK_SCALING,
    DEST_EG3_ATTACK_SCALING,
    DEST_EG4_ATTACK_SCALING,
    DEST_ALL_EG_ATTACK_SCALING,
    DEST_EG1_DECAY_SCALING,
    DEST_EG2_DECAY_SCALING,
    DEST_EG3_DECAY_SCALING,
    DEST_EG4_DECAY_SCALING,
    DEST_ALL_EG_DECAY_SCALING,
    DEST_EG1_SUSTAIN_OVERRIDE,
    DEST_EG2_SUSTAIN_OVERRIDE,
    DEST_EG3_SUSTAIN_OVERRIDE,
    DEST_EG4_SUSTAIN_OVERRIDE,
    DEST_ALL_EG_SUSTAIN_OVERRIDE, // <- keep this last
    // --- END OF LAYER 0 DESTINATIONS

    // --- LAYER 1 DESTINATIONS
            DEST_HARD_SYNC_RATIO,
    DEST_OSC1_FO,
    DEST_OSC2_FO,
    DEST_OSC3_FO,
    DEST_OSC4_FO,
    DEST_ALL_OSC_FO,
    DEST_OSC1_PULSEWIDTH,
    DEST_OSC2_PULSEWIDTH,
    DEST_OSC3_PULSEWIDTH,
    DEST_OSC4_PULSEWIDTH,
    DEST_ALL_OSC_PULSEWIDTH,
    DEST_OSC1_FO_RATIO,
    DEST_OSC2_FO_RATIO,
    DEST_OSC3_FO_RATIO,
    DEST_OSC4_FO_RATIO,
    DEST_ALL_OSC_FO_RATIO,
    DEST_OSC1_OUTPUT_AMP,
    DEST_OSC2_OUTPUT_AMP,
    DEST_OSC3_OUTPUT_AMP,
    DEST_OSC4_OUTPUT_AMP,
    DEST_ALL_OSC_OUTPUT_AMP,
    DEST_FILTER1_FC,
    DEST_FILTER2_FC,
    DEST_ALL_FILTER_FC,
    DEST_FILTER1_KEYTRACK,
    DEST_FILTER2_KEYTRACK,
    DEST_ALL_FILTER_KEYTRACK, // the control value, overriding the GUI
    DEST_FILTER1_Q,
    DEST_FILTER2_Q,
    DEST_ALL_FILTER_Q,
    DEST_VS_AC_AXIS,
    DEST_VS_BD_AXIS,
    DEST_DCA_VELOCITY,
    DEST_DCA_EG,
    DEST_DCA_AMP,
    DEST_DCA_PAN,
    // --- END OF LAYER 1 DESTINATIONS

    MAX_DESTINATIONS
};
struct modMatrixRow
{
    // --- index of source
    UINT uSourceIndex;

    // --- index of destination
    UINT uDestinationIndex;

    // --- for GUI -> Control
    double dUIContolValue;

    // --- needs to default to 1.0 in case no GUI control
    double* pModIntensity;	// <- "ucControl" in MMA DLS
    double* pModRange;		// <- "lScale" in MMA DLS

    // --- transform on Source
    UINT uSourceTransform;

    // --- to easily turn on/off a modulation routing
    bool bEnable;
};


/* unipolarToBipolar()
	
	calculates the bipolar (-1 -> +1) value from a unipolar (0 -> 1) value
	
	dValue = the value to convert
*/
inline double unipolarToBipolar(double dValue)
{
    return 2.0*dValue - 1.0;
}

inline double bipolarToUnipolar(double dValue)
{
    return 0.5*dValue + 0.5;
}

inline modMatrixRow** createModMatrixCore()
{
    modMatrixRow** pCore = new modMatrixRow*[MAX_SOURCES*MAX_DESTINATIONS];
    memset(pCore, 0, MAX_SOURCES*MAX_DESTINATIONS*sizeof(modMatrixRow*));
    return pCore;
}


enum transform{
    TRANSFORM_NONE = 0,
    TRANSFORM_UNIPOLAR_TO_BIPOLAR,
    TRANSFORM_BIPOLAR_TO_UNIPOLAR,
    TRANSFORM_MIDI_NORMALIZE,
    TRANSFORM_INVERT_MIDI_NORMALIZE,
    TRANSFORM_MIDI_TO_BIPOLAR,
    TRANSFORM_MIDI_TO_PAN,
    TRANSFORM_MIDI_SWITCH,
    TRANSFORM_MIDI_TO_ATTENUATION,
    TRANSFORM_NOTE_NUMBER_TO_FREQUENCY,
    MAX_TRANSFORMS /* not needed? */
};


inline double paramToTimeCents(double dParam)
{
    return 1200.0*65536.0*fastlog2(dParam);
}

/* timeCentsToParam()
	
	converts a timeCents to a time (sec) value
	See the MMA MIDI DLS Level 1 or 2 Spec

*/
inline double timeCentsToParam(double dTimeCents)
{
    //return fastpow2(dTimeCents/(1200.0*65536.0));
    // return fastpow2(dTimeCents/(1200.0*65536.0));
    return pow(2.0, dTimeCents/(1200.0*65536.0));
}

inline double mmaMIDItoParamScaling(UINT uMIDIValue, float fParameter, float fScaling)
{
    return timeCentsToParam(paramToTimeCents(fScaling)*(double(uMIDIValue)/128.0) + paramToTimeCents(fParameter));
}

/* mmaMIDItoAtten_dB()
	
	calculates the dB of attenuation according to MMA DLS spec
	
	uMIDIValue = the MIDI (0 -> 127) value to convert
*/
inline double mmaMIDItoAtten_dB(UINT uMIDIValue)
{
    if(uMIDIValue == 0)
        return -96.0; // dB floor

    return 20.0*log10((127.0*127.0)/((float)uMIDIValue*(float)uMIDIValue));
}


static inline float fasttan (float x)
{
    static const float halfpi = 1.5707963267948966f;
    return sin (x) / sin (x + halfpi);
}

static inline float fasttanh (float p)
{
//  return -1.0f + 2.0f / (1.0f + fastexp (-2.0f * p));
    return p/(fabs(2*p)+3/(2+2*p*2*p));
}
inline double midiToBipolar(UINT uMIDIValue)
{
    return 2.0*(double)uMIDIValue/127.0 - 1.0;
}
/* mmaMIDItoAtten()
	
	calculates the raw attenuation according to MMA DLS spec
	
	uMIDIValue = the MIDI (0 -> 127) value to convert
*/
inline double mmaMIDItoAtten(UINT uMIDIValue)
{
    if(uMIDIValue == 0)
        return 0.0; // floor

    return ((double)uMIDIValue*(double)uMIDIValue)/(127.0*127.0);;
}
inline double midiToPanValue(UINT uMIDIValue)
{
    // see MMA DLS Level 2 Spec; controls are asymmetrical
    if(uMIDIValue == 64)
        return 0.0;
    else if (uMIDIValue <= 1) // 0 or 1
        return -1.0;

    return 2.0*(double)uMIDIValue/127.0 - 1.0;
}