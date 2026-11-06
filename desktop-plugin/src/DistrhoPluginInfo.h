//
// DPF's description of the plugin: one instrument, stereo out, MIDI in, no custom
// UI yet (hosts show their own generic editor for the parameters).
//

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

#define DISTRHO_PLUGIN_BRAND "Sufy Studios"
#define DISTRHO_PLUGIN_NAME  "Drone Synth"
#define DISTRHO_PLUGIN_URI   "https://sufystudios.xyz/plugins/dronesynth"
#define DISTRHO_PLUGIN_CLAP_ID "xyz.sufystudios.dronesynth"

// Four-character codes for AU and VST3. Never change them once released:
// hosts use them to find the plugin in saved projects.
#define DISTRHO_PLUGIN_BRAND_ID Sufy
#define DISTRHO_PLUGIN_UNIQUE_ID DrSy

#define DISTRHO_PLUGIN_HAS_UI          0
#define DISTRHO_PLUGIN_IS_RT_SAFE      1
#define DISTRHO_PLUGIN_IS_SYNTH        1
#define DISTRHO_PLUGIN_NUM_INPUTS      0
#define DISTRHO_PLUGIN_NUM_OUTPUTS     2
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT 1

#define DISTRHO_PLUGIN_AU_TYPE aumu
#define DISTRHO_PLUGIN_VST3_CATEGORIES "Instrument|Synth|Stereo"
#define DISTRHO_PLUGIN_CLAP_FEATURES "instrument", "synthesizer", "stereo"
#define DISTRHO_PLUGIN_LV2_CATEGORY "lv2:InstrumentPlugin"

#endif
