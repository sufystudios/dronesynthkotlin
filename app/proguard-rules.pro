# JNI: the native engine looks these up by name.
-keep class xyz.sufystudios.synthguide.audio.NativeSynth { *; }

# Play's review-ktx references this annotation, which isn't shipped with it. It has no runtime effect.
-dontwarn com.google.android.gms.common.annotation.NoNullnessRewrite
