//
// Created by frederickb on 18/6/21.
//

#ifndef DIDGERIDOOSYNTH_TUNIN_H
#define DIDGERIDOOSYNTH_TUNIN_H

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
#endif //DIDGERIDOOSYNTH_TUNIN_H
