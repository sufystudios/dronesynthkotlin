package xyz.sufystudios.didgsynth;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.media.AudioManager;
import android.os.Build;
import android.os.Bundle;


import android.text.Editable;
import android.text.TextWatcher;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;

import android.widget.EditText;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;


import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.anjlab.android.iab.v3.BillingProcessor;
import com.anjlab.android.iab.v3.TransactionDetails;

//import com.kobakei.ratethisapp.RateThisApp;

import java.util.List;

import it.beppi.knoblibrary.Knob;

import com.google.android.material.button.MaterialButton;
import static android.view.MotionEvent.ACTION_DOWN;
import static android.view.MotionEvent.ACTION_UP;
import static xyz.sufystudios.didgsynth.R.*;
//import xyz.murdoch.edu.au.learnsynth.R;

public class MainActivity extends AppCompatActivity implements BillingProcessor.IBillingHandler {
    static long mEngineHandle = 0;

    // Load native library

    static {
        System.loadLibrary("native-lib");
    }

    Context context;

    private BillingProcessor bp;

    String aiDroneString = "ai_drone_upgrade";

    boolean AI_PURHCASED=false;

    boolean AION=false;

    MaterialButton  aiDrone;

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (!bp.handleActivityResult(requestCode, resultCode, data)) {
            super.onActivityResult(requestCode, resultCode, data);
        }
    }

    protected void toggleAIDrone() {
        List<String> products =bp.listOwnedProducts();
        for(String i : products) {
            if(i.equals(aiDroneString)) {
                AI_PURHCASED =true;
            }

        }
        if(!AI_PURHCASED) {
            bp.purchase(this,aiDroneString);

        }
        if(AI_PURHCASED && !AION) {
            aiDrone.setText("AI:ON");
            AION=true;
            changeKnob(21,1);
        }
        else{
            aiDrone.setText("AI:OFF");
            AION=false;
            changeKnob(21,0);
        }
    }

    private void createStuff() {

        //startEngine();
        bp = new BillingProcessor(this, "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAp60Ho4VkC75TR1HXwwbYK/1MIMP2vhE8S6LyHIY6WdAWuThV8czkvzlGv3HaExH1bbVUl3k5jKYvS+JLIyuHCY7bSFLKs/nQTI8ZynbEElvmnVL7sW84LlRLA32/K1u1YNligjY6b8m20hroZZNuSavaHpn6foSfrLaIXMIKBLCQEqmhd+V5Y3xSVCF2/5b0ACPyG95GYcquEuX5N93/42i5xVb3ZotBrclwQf/9tIfMne3BDcWJFS7rQOlpNM0+QrW1Q9dTUP3bLVkEHRbtyhPUSBa5O03c0lmil+I/86TnO24Y5d0+A64EWnvIiP/+AtA2Ud62htGOhKVLWtK6cQIDAQAB", this);
        bp.initialize();
        MaterialButton  a = findViewById(id.a);
        a.setOnTouchListener(motionListener);
        aiDrone = findViewById(id.aibutton );
        aiDrone.setText("AI:OFF");
        aiDrone.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                toggleAIDrone();
            }
        });

        MaterialButton  asharp = findViewById(id.asharp);

        asharp.setOnTouchListener(motionListener);

        MaterialButton  b = findViewById(id.b);
        b.setOnTouchListener(motionListener);

        MaterialButton  c = findViewById(id.c);
        c.setOnTouchListener(motionListener);

        MaterialButton  csharp = findViewById(id.csharp);
        csharp.setOnTouchListener(motionListener);

        MaterialButton  d = findViewById(id.d);
        d.setOnTouchListener(motionListener);

        MaterialButton  dsharp = findViewById(id.dsharp);
        dsharp.setOnTouchListener(motionListener);

        MaterialButton  e = findViewById(id.e);
        e.setOnTouchListener(motionListener);

        MaterialButton  f = findViewById(id.f);
        f.setOnTouchListener(motionListener);

        MaterialButton  fsharp = findViewById(id.fsharp);
        fsharp.setOnTouchListener(motionListener);

        MaterialButton  g = findViewById(id.g);
        g.setOnTouchListener(motionListener);

        MaterialButton  gsharp = findViewById(id.gsharp);
        gsharp.setOnTouchListener(motionListener);



//        MaterialButton  a = (MaterialButton ) findViewById(R.id.a);
//        a.setOnGenericMotionListener(motionListener);
//        MaterialButton  asharp = (MaterialButton ) findViewById(R.id.asharp);
//        asharp.setOnGenericMotionListener(motionListener);
//        MaterialButton  b = (MaterialButton ) findViewById(R.id.b);
//        b.setOnGenericMotionListener(motionListener);
//        MaterialButton  c = (MaterialButton ) findViewById(R.id.c);
//        c.setOnGenericMotionListener(motionListener);
//        MaterialButton  csharp = (MaterialButton ) findViewById(R.id.csharp);
//        csharp.setOnGenericMotionListener(motionListener);
//        MaterialButton  d = (MaterialButton ) findViewById(R.id.d);
//        d.setOnGenericMotionListener(motionListener);
//        MaterialButton  dsharp = (MaterialButton ) findViewById(R.id.dsharp);
//        dsharp.setOnGenericMotionListener(motionListener);
//        MaterialButton  e = (MaterialButton ) findViewById(R.id.e);
//        e.setOnGenericMotionListener(motionListener);
//        MaterialButton  f = (MaterialButton ) findViewById(R.id.f);
//        f.setOnGenericMotionListener(motionListener);
//        MaterialButton  fsharp = (MaterialButton ) findViewById(R.id.fsharp);
//        fsharp.setOnGenericMotionListener(motionListener);
//        MaterialButton  g = (MaterialButton ) findViewById(R.id.g);
//        g.setOnGenericMotionListener(motionListener);
//        MaterialButton  gsharp = (MaterialButton ) findViewById(R.id.gsharp);
//        gsharp.setOnGenericMotionListener(motionListener);
//        MaterialButton Rect = new MaterialButton [12];
//
//        MaterialButton Rect[0] = a;
//
//        MaterialButton Rect[1] = asharp;
//        MaterialButton Rect[2] = b;
//        MaterialButton Rect[3] = c;
//        MaterialButton Rect[4] = csharp;
//        MaterialButton Rect[5] = d;
//        MaterialButton Rect[6] = dsharp;
//        MaterialButton Rect[7] = e;
//        MaterialButton Rect[8] = f;
//        MaterialButton Rect[9] = fsharp;
//        MaterialButton Rect[10] = g;
//        MaterialButton Rect[11] = gsharp;
////wavetype=(TextView) findViewById(R.id.wavetype);
        final TextView oct = findViewById(id.oct);
        oct.setText("0");
        MaterialButton  down = findViewById(id.octdown);
        down.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (octint > -3) {
                    octint--;
                    Integer intd = new Integer(octint);
                    oct.setText(intd.toString());
                }
            }
        });
        MaterialButton  up = findViewById(id.octup);
        up.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (octint < 3) {
                    octint++;
                    Integer intd = new Integer(octint);
                    oct.setText(intd.toString());
                }
            }
        });
        Knob attack = findViewById(id.attack);
        attack.setState(50);
        attack.setOnStateChanged(new Knob.OnStateChanged() {
            @Override
            public void onState(int state) {
                // do something
                changeKnob(0, state);
            }
        });
        Knob decay = findViewById(id.decay);
        decay.setState(0);
        decay.setOnStateChanged(new Knob.OnStateChanged() {
            @Override
            public void onState(int state) {
                // do something
                changeKnob(1, state);
            }
        });
        Knob sustain = findViewById(id.sustain);
        sustain.setState(0);
        sustain.setOnStateChanged(new Knob.OnStateChanged() {
            @Override
            public void onState(int state) {
                // do something
                changeKnob(2, state);
            }
        });
        Knob release = findViewById(id.release);
        release.setState(0);
        release.setOnStateChanged(new Knob.OnStateChanged() {
            @Override
            public void onState(int state) {
                changeKnob(3, state);
                // do something
            }
        });
        filter = findViewById(id.filter);
        filter.setProgress(100);
        changeKnob(4, 100);
        filter.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
                changeKnob(4, i);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });

        SeekBar Q = findViewById(id.Q);
        changeKnob(5, 20);
        Q.setProgress(20);
        Q.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
                changeKnob(5, i);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });
        SeekBar q = findViewById(id.q);
        changeKnob(19, 23);
        q.setProgress(23);
        q.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
                changeKnob(19, i);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });
        Knob drywet = findViewById(id.drywet);
        drywet.setState(20);
        changeKnob(6,20);
        drywet.setOnStateChanged(new Knob.OnStateChanged() {
            @Override
            public void onState(int state) {
                // do something
                changeKnob(6, state);
            }
        });
        Knob time = findViewById(id.delaytime);
        time.setState(49);
        changeKnob(7,49);
        time.setOnStateChanged(new Knob.OnStateChanged() {
            @Override
            public void onState(int state) {
                // do something
                changeKnob(7, state);
            }
        });
        Knob delayratio = findViewById(id.delayratio);
        delayratio.setState(18);
        changeKnob(8,18);
        delayratio.setOnStateChanged(new Knob.OnStateChanged() {
            @Override
            public void onState(int state) {
                // do something
                changeKnob(8, state);
            }
        });
        Knob feedback = findViewById(id.feedback);
        feedback.setState(20);
        changeKnob(9,20);
        feedback.setOnStateChanged(new Knob.OnStateChanged() {
            @Override
            public void onState(int state) {
                // do something
                changeKnob(9, state);
            }
        });
        SeekBar ratelfo = findViewById(id.LFORate);
        ratelfo.setProgress(70);
        changeKnob(10,50);
        ratelfo.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                changeKnob(10, progress);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });
        SeekBar reverb = findViewById(id.reverb);
        reverb.setProgress(20);
        changeKnob(14,20);
        reverb.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                changeKnob(14, progress);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });
        final MaterialButton  randomlfo = findViewById(id.randlfo);
        randomlfo.setBackgroundColor(Color.RED);
        randomlfo.setOnClickListener(new View.OnClickListener() {
            private boolean randlfo = false;

            @SuppressLint({"ResourceAsColor", "ResourceType"})
            @Override
            public void onClick(View view) {
                if (!randlfo) {

                    randomlfo.setBackgroundColor(Color.GREEN);
                    changeKnob(17, 1);
                    randlfo = true;
                } else {
                    randomlfo.setBackgroundColor(Color.RED);
                    changeKnob(17, 0);
                    randlfo = false;
                }
            }
        });
        final MaterialButton  randomovertones = findViewById(id.randomovertones);
        randomovertones.setBackgroundColor(Color.RED);
        randomovertones.setOnClickListener(new View.OnClickListener() {
            private boolean randover = false;

            @SuppressLint({"ResourceAsColor", "ResourceType"})
            @Override
            public void onClick(View view) {
                if (!randover) {

                    randomovertones.setBackgroundColor(Color.GREEN);
                    changeKnob(18, 1);
                    randover = true;
                } else {
                    randomovertones.setBackgroundColor(Color.RED);
                    changeKnob(18, 0);
                    randover = false;
                }
            }
        });
        final MaterialButton  looper = findViewById(id.looper);
        looper.setBackgroundColor(Color.RED);
        looper.setOnClickListener(new View.OnClickListener() {
            private boolean loop = false;

            @SuppressLint({"ResourceAsColor", "ResourceType"})
            @Override
            public void onClick(View view) {
                if (!loop) {

                    looper.setBackgroundColor(Color.GREEN);
                    changeKnob(20, 1);
                    loop = true;
                } else {
                    looper.setBackgroundColor(Color.RED);
                    changeKnob(20, 0);
                    loop = false;
                }
            }
        });
        Knob lfopitch = findViewById(id.LFOPitch);
        lfopitch.setState(2);
        changeKnob(11,2);
        lfopitch.setOnStateChanged(new Knob.OnStateChanged() {
            @Override
            public void onState(int state) {
                // do something
                changeKnob(11, state);
            }
        });

        Knob lfofilter = findViewById(id.LFOFilter);
        lfofilter.setState(5);
        changeKnob(12,5);
        lfofilter.setOnStateChanged(new Knob.OnStateChanged() {
            @Override
            public void onState(int state) {
                // do something
                changeKnob(12, state);
            }
        });
        drone = findViewById(id.drone);
        if (dronebool == false)
            changeKnob(13, 0);
        drone.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (dronebool) {
                    droneint = 0;
                    dronebool = false;
                    changeKnob(13, 0);
                    changeKnob(16, 0);
                    drone.setText("Drone OFF");

                } else {
                    droneint = 1;
                    dronebool = true;
                    changeKnob(13, 1);
                    drone.setText("Drone ON");

                }
            }
        });
        final MaterialButton  selectwave = findViewById(id.selectWave);
        selectwave.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                currentWave = (currentWave + 1) % waves.length;
                selectwave.setText(waves[currentWave]);
                changeKnob(15, currentWave);

            }
        });
        guide = findViewById(id.guide);
        guide.setMovementMethod(new ScrollingMovementMethod());
        guide.setText(guidetxt[0]);
        final MaterialButton  next = findViewById(id.next);
        final MaterialButton  prev = findViewById(id.prev);
        MaterialButton  hide = findViewById(id.hidehelp);
        hide.setOnClickListener(new View.OnClickListener() {
            Boolean helphidden = false;

            @Override
            public void onClick(View view) {
                if (!helphidden) {
                    guide.setVisibility(view.GONE);
                    next.setVisibility(view.GONE);
                    prev.setVisibility(view.GONE);
                    helphidden = true;
                } else {
                    guide.setVisibility(view.VISIBLE);
                    next.setVisibility(view.VISIBLE);
                    prev.setVisibility(view.VISIBLE);
                    helphidden = false;
                }
            }
        });
        next.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (currentins < guidetxt.length - 1) {
                    currentins++;
                }
                displayinst();
            }
        });
        prev.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (currentins > 0) {
                    currentins--;
                }
                displayinst();
            }
        });
        final EditText hertz = (EditText) findViewById(id.hertz);
        final ImageView tube = (ImageView) findViewById(id.tube);
        tube.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {

                ImageView tube = (ImageView) view;
                if (!tubeoff) {
                    tube.setColorFilter(Color.rgb(10, 200, 150), PorterDuff.Mode.SRC_ATOP);
                    tube.setAlpha((float) 0.2);
                    native_setTube(1);
                    tubeoff = true;
                } else {
                    tubeoff = false;
                    tube.clearColorFilter();
                    tube.setAlpha((float) 1);
                    native_setTube(1);

                }
            }
        });
        hertz.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {

            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {

            }

            @Override
            public void afterTextChanged(Editable s) {
                try {
                    double test = Double.parseDouble(hertz.getText().toString());
                    changeKnobDouble(16, test);


                } catch (Exception e) {
                    Toast toast = Toast.makeText(getApplicationContext(), "Not a number", Toast.LENGTH_SHORT);
                    toast.show();
                    //EditText.setValue();
                }
            }
        });


    }

    static boolean create(Context context) {

        if (mEngineHandle == 0) {
            setDefaultStreamValues(context);

        }
        return (mEngineHandle != 0);
    }

    private static void setDefaultStreamValues(Context context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            AudioManager myAudioMgr = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
            String sampleRateStr = myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
            int defaultSampleRate = Integer.parseInt(sampleRateStr);
            String framesPerBurstStr = myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
            int defaultFramesPerBurst = Integer.parseInt(framesPerBurstStr);


        }
    }

    private int octint;
    private int currentins = 0;
    private boolean dronebool = false;
    private boolean tubeon = false;
    private static final int MAXINST = 4;
    public TextView guide;
    private String[] guidetxt = {"Welcome to Didg Synth, a simple synth to jam along with or record using an audio interface\n Click next to continue reading instructions for use.", "Attack is the time for the note to get to max volume\nDecay is time from max attack to sustain level\nSustain is level when the note is held for a long period\nRelease is how long for volume to go down as it is released. ",

            "LFO rate is the speed of the low frequency wave that can affect the pitch or the filter\nLFO Filt is how much it affects the filter\nLFO pitch how much it affects oscillator\n",

            "Delay and reverb sound good with the drone experiment \nFor delay turn up the delay time, delay wet dry to about 50%, \nratio is the time for each channel either left or right, you can ballance them or make the delay higher in one channel\n, as well as feedback, \nFor reverb turn up the wet dry below this text.",

            "To play Drone press drone MaterialButton  then select your note.\nTo go to a different octave select the MaterialButton  for it.",
            "To do a sine wave FM synthesis using one of the 3 oscillators as a modifier for the other frequencies but detuned slightly press the MaterialButton  with the wave name until it says FM sine.\n" +

                    "to go back to regular synth mode press the oscillator waveform MaterialButton  again",
            "Oscillator types, There is Saw Square and Triangle aswell as FM sinusoid. The sinusoid is a pure tone without hamonies, square and saw have alot of harmonies , Noise can sound like ocean waves or wind, triangle has less but its not just one like the sine wave so its a softer tone.",


    };

    private static native void native_setTube(int tube);


    private native void touchEvent(int action, int key);


    private native void changeKnob(int knob, int val);

    private native void changeKnobDouble(int knob, double val);

    private TextView mLatencyText;


    private int mSlop;
    private float mDownX;
    private float mDownY;
    private boolean mSwiping;
    private boolean tubeoff;
    private SeekBar filter;
    private String[] waves = {"Harmony", "Drone"};
    private int currentWave = 0;
    private View lastview;

    View.OnTouchListener motionListener = new View.OnTouchListener() {
        boolean isplaying = false;

        @Override
        public boolean onTouch(View v, MotionEvent event) {
            switch (event.getActionMasked()) {

                case ACTION_DOWN:

                    doKeys(0, v);


                    return true;



                case ACTION_UP:

                    doKeys(2, v);
                    // doKeys(2, v);

                    return true;

            }
            return false;
        }

    };
    //private class TouchListener implements View.OnClickListener {
//    @Override
//    public void onClick(View view) {
//        doKeys;
//    }
//}
    double hertz = 440;
    private TextView wavetype;
    //    private MaterialButton [] MaterialButton Rect;
    private MaterialButton  drone;

    @Override
    protected void onPause() {

        PlaybackEngine.stop();
        super.onPause();
    }


    @Override
    protected void onResume() {

        PlaybackEngine.start();
        Log.d("on resume","on resume");
        super.onResume();
        //   createStuff();


    }
    Activity thisactivity;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        context = this.getApplicationContext();
        //getSupportActionBar().hide(); //<< this
        thisactivity=this;
        setContentView(layout.activity_main);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        PlaybackEngine.create(this);
        createStuff();


//        RateThisApp.onCreate(this);
//        RateThisApp.Config config = new RateThisApp.Config(2, 3);
//        RateThisApp.init(config);
//        RateThisApp.showRateDialogIfNeeded(this);


    }

//    @Override
//    public boolean onTouchEvent(MotionEvent event) {
//        touchEvent(event.getAction());
//        return super.onTouchEvent(event);
//    }


    @Override
    protected void onDestroy() {

        if (bp != null) {
            bp.release();
        }

        PlaybackEngine.delete();
        super.onDestroy();
    }

    private void displayinst() {
        guide.setText(guidetxt[currentins]);
    }

    public void nextinst() {

    }

    public void previnst() {


    }

    private Rect rect;    // Variable rect to hold the bounds of the view

    private boolean checkInterSection(MaterialButton  view, float rawX, float rawY) {

//                       // View t=findViewById(R.id.content);
        Rect slide = new Rect(view.getLeft(), view.getTop() + view.getHeight() / 2, view.getRight(), view.getBottom() + view.getHeight() / 2);
        return slide.contains((int) rawX, (int) rawY);
    }

    MaterialButton last;
    boolean noteon = false;
//    @Override
//    public boolean onTouch(View v, MotionEvent event) {
//       // Log.d("tesst", "test");
//
//boolean overlap=false;
//        switch (event.getActionMasked()) {
//            // Construct a rect of the view's bounds
//
//
//
//
//
//
//
//            case MotionEvent.ACTION_DOWN:
//
//                for (int i = 0; i < MaterialButton Rect.length; i++) {
//                    if (
//                            checkInterSection(MaterialButton Rect[i], event.getRawX(), event.getRawY())
//                    ) {
//
//                        if(MaterialButton last==null ||MaterialButton Rect[i].getId()!=MaterialButton last.getId()) {
//                           // Log.d("intersection","" +i);
//                            Log.d("intersection","" +i);
//                            MaterialButton last = MaterialButton Rect[i];
//                            doKeys(0,MaterialButton last);
//                            noteon=true;
//
//                        } else {
//                            if(noteon!=true) {
//                                doKeys(0, MaterialButton Rect[i]);
//                                MaterialButton last = MaterialButton Rect[i];
//                            } noteon=true;
//                        }
//
//        return false;
//
//                    }
//
//
//                }
//
//
//
//
//            case MotionEvent.ACTION_UP:
//                doKeys(event, v);
//                MaterialButton last=null;
//                noteon=false;
//                return false;
//
//
////            if ((rect!=null && !rect.contains(v.getLeft() + (int) event.getX(), v.getTop() + (int) event.getY()) )|| rect==null) {
////                // User moved outside bounds
////
////                boolean found=false;
////                    for(int i=0;i<12;i++) {
////                        View p=MaterialButton Rect[i];
////                       // View t=findViewById(R.id.content);
////                       Rect slide= new Rect(p.getLeft(),p.getTop(),p.getRight(),p.getBottom());
////                       if(slide.contains(p.getLeft() + (int) event.getX(), p.getTop() + (int) event.getY())) {
////                           found=true;
////                           rect=slide;
////                           doKeys(2,p);
////                           doKeys(0,p);
////                           return false;
////
////                       }
////}
////
////                    }
////                touchEvent(2,0);
////
////
////                rect=null;
//            default:
//
//
//        }
//        touchEvent(2,0);
//        MaterialButton last=null;
//        return false;
//
//    }

    int droneint;

    public void doKeys(int event, View v) {
        switch (v.getId()) {

            case id.a:
                touchEvent(event + (droneint), 12 * octint + 69);
                break;
            case id.b:
                touchEvent(event + droneint, 12 * octint + 71);
                break;
            case id.c:
                touchEvent(event + droneint, 12 * octint + 60);
                break;
            case id.d:
                touchEvent(event + droneint, 12 * octint + 62);
                break;
            case id.e:
                touchEvent(event + droneint, 12 * octint + 64);
                break;
            case id.f:
                touchEvent(event + droneint, 12 * octint + 65);
                break;
            case id.g:
                touchEvent(event + droneint, 12 * octint + 67);
                break;
            case id.asharp:
                touchEvent(event + droneint, 12 * octint + 70);
                break;
            case id.csharp:
                touchEvent(event + droneint, 12 * octint + 61);
                break;
            case id.dsharp:
                touchEvent(event + droneint, 12 * octint + 63);
                break;
            case id.fsharp:
                touchEvent(event + droneint, 12 * octint + 66);
                break;
            case id.gsharp:
                touchEvent(event + droneint, 12 * octint + 68);
                break;
//                default:
//                    touchEvent(2, 12 * octint + 68);

        }
    }

    public void doKeys(MotionEvent event, View v) {
        switch (v.getId()) {

            case id.a:
                touchEvent(event.getActionMasked(), 12 * octint + 69);
                break;
            case id.b:
                touchEvent(event.getActionMasked(), 12 * octint + 71);
                break;
            case id.c:
                touchEvent(event.getActionMasked(), 12 * octint + 60);
                break;
            case id.d:
                touchEvent(event.getActionMasked(), 12 * octint + 62);
                break;
            case id.e:
                touchEvent(event.getActionMasked(), 12 * octint + 64);
                break;
            case id.f:
                touchEvent(event.getActionMasked(), 12 * octint + 65);
                break;
            case id.g:
                touchEvent(event.getActionMasked(), 12 * octint + 67);
                break;
            case id.asharp:
                touchEvent(event.getActionMasked(), 12 * octint + 70);
                break;
            case id.csharp:
                touchEvent(event.getActionMasked(), 12 * octint + 61);
                break;
            case id.dsharp:
                touchEvent(event.getActionMasked(), 12 * octint + 63);
                break;
            case id.fsharp:
                touchEvent(event.getActionMasked(), 12 * octint + 66);
                break;
            case id.gsharp:
                touchEvent(event.getActionMasked(), 12 * octint + 68);
                break;
//                default:
//                    touchEvent(2, 12 * octint + 68);

        }
    }

    Rect crect;
    Rect drect;
    Rect erect;
    Rect frect;
    Rect grect;
    Rect arect;
    Rect brect;
    Rect csharprect;
    Rect dsharprect;
    Rect fsharprect;
    Rect gsharprect;
    Rect asharprect;
    boolean isdown = false;


    @Override
    public void onProductPurchased(@NonNull String productId, @Nullable TransactionDetails details) {
        if(productId.equals(aiDroneString)) {
            AI_PURHCASED=true;
            toggleAIDrone();
        }
    }

    @Override
    public void onPurchaseHistoryRestored() {

    }

    @Override
    public void onBillingError(int errorCode, @Nullable Throwable error) {

    }

    @Override
    public void onBillingInitialized() {

    }
/*
    @Override
    public boolean onTouch(View v, MotionEvent me) {



        switch (me.getActionMasked()) {
            // Construct a rect of the view's bounds





            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_MOVE:
                Log.d("Text", "TEST %d %d"+me.getX()+me.getY());{if(!noteon) {
                if (arect.contains((int) me.getX(), (int) me.getY())) {
                    doKeys(0, MaterialButton Rect[0]);
                    noteon = true;

                } else if (brect.contains((int) me.getX(), (int) me.getY())) {
                    doKeys(0, MaterialButton Rect[2]);
                    noteon = true;

                } else if (crect.contains((int) me.getX(), (int) me.getY())) {
                    doKeys(0, MaterialButton Rect[3]);
                    noteon = true;

                } else if (drect.contains((int) me.getX(), (int) me.getY())) {
                    doKeys(0, MaterialButton Rect[5]);
                    noteon = true;

                } else if (erect.contains((int) me.getX(), (int) me.getY())) {
                    doKeys(0, MaterialButton Rect[7]);
                    noteon = true;

                } else if (frect.contains((int) me.getX(), (int) me.getY())) {
                    doKeys(0, MaterialButton Rect[8]);
                    noteon = true;

                } else if (grect.contains((int) me.getX(), (int) me.getY())) {
                    doKeys(0, MaterialButton Rect[10]);
                    noteon = true;

                } else if (asharprect.contains((int) me.getX(), (int) me.getY())) {
                    doKeys(0, MaterialButton Rect[1]);
                    noteon = true;

                } else if (gsharprect.contains((int) me.getX(), (int) me.getY())) {
                    doKeys(0, MaterialButton Rect[11]);
                    noteon = true;

                } else if (fsharprect.contains((int) me.getX(), (int) me.getY())) {
                    doKeys(0, MaterialButton Rect[9]);
                    noteon = true;

                } else if (dsharprect.contains((int) me.getX(), (int) me.getY())) {
                    doKeys(0, MaterialButton Rect[6]);
                    noteon = true;

                } else if (csharprect.contains((int) me.getX(), (int) me.getY())) {
                    doKeys(0, MaterialButton Rect[4]);
                    noteon = true;

                }

                return true;


            }}
                case MotionEvent.ACTION_UP:
                    doKeys(2,MaterialButton Rect[4]);
                    noteon=false;







return true;


            default:


        }
        touchEvent(2,0);
        MaterialButton last=null;
        return false;

    }

 */

}
