package xyz.sufystudios.didgsynth;

import android.content.Context;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.media.AudioManager;
import android.os.Build;
import android.os.Bundle;

import android.text.Editable;
import android.text.TextWatcher;
import android.text.method.ScrollingMovementMethod;

import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

import androidx.appcompat.app.AppCompatActivity;

import com.kobakei.ratethisapp.RateThisApp;

import it.beppi.knoblibrary.Knob;

import static android.view.MotionEvent.ACTION_DOWN;

import static android.view.MotionEvent.ACTION_UP;
//import xyz.murdoch.edu.au.learnsynth.R;

public class MainActivity extends AppCompatActivity {
    static long mEngineHandle = 0;

    // Load native library
private void createStuff() {
    PlaybackEngine.create(this);
    //startEngine();
    Button a = findViewById(R.id.a);
    a.setOnTouchListener(motionListener);
    Button asharp = findViewById(R.id.asharp);
    asharp.setOnTouchListener(motionListener);
    Button b = findViewById(R.id.b);
    b.setOnTouchListener(motionListener);
    Button c = findViewById(R.id.c);
    c.setOnTouchListener(motionListener);
    Button csharp = findViewById(R.id.csharp);
    csharp.setOnTouchListener(motionListener);
    Button d = findViewById(R.id.d);
    d.setOnTouchListener(motionListener);
    Button dsharp = findViewById(R.id.dsharp);
    dsharp.setOnTouchListener(motionListener);
    Button e = findViewById(R.id.e);
    e.setOnTouchListener(motionListener);
    Button f = findViewById(R.id.f);
    f.setOnTouchListener(motionListener);
    Button fsharp = findViewById(R.id.fsharp);
    fsharp.setOnTouchListener(motionListener);
    Button g = findViewById(R.id.g);
    g.setOnTouchListener(motionListener);
    Button gsharp = findViewById(R.id.gsharp);
    gsharp.setOnTouchListener(motionListener);
//        Button a = (Button) findViewById(R.id.a);
//        a.setOnGenericMotionListener(motionListener);
//        Button asharp = (Button) findViewById(R.id.asharp);
//        asharp.setOnGenericMotionListener(motionListener);
//        Button b = (Button) findViewById(R.id.b);
//        b.setOnGenericMotionListener(motionListener);
//        Button c = (Button) findViewById(R.id.c);
//        c.setOnGenericMotionListener(motionListener);
//        Button csharp = (Button) findViewById(R.id.csharp);
//        csharp.setOnGenericMotionListener(motionListener);
//        Button d = (Button) findViewById(R.id.d);
//        d.setOnGenericMotionListener(motionListener);
//        Button dsharp = (Button) findViewById(R.id.dsharp);
//        dsharp.setOnGenericMotionListener(motionListener);
//        Button e = (Button) findViewById(R.id.e);
//        e.setOnGenericMotionListener(motionListener);
//        Button f = (Button) findViewById(R.id.f);
//        f.setOnGenericMotionListener(motionListener);
//        Button fsharp = (Button) findViewById(R.id.fsharp);
//        fsharp.setOnGenericMotionListener(motionListener);
//        Button g = (Button) findViewById(R.id.g);
//        g.setOnGenericMotionListener(motionListener);
//        Button gsharp = (Button) findViewById(R.id.gsharp);
//        gsharp.setOnGenericMotionListener(motionListener);
    buttonRect = new Button[12];

    buttonRect[0] = a;

    buttonRect[1] = asharp;
    buttonRect[2] = b;
    buttonRect[3] = c;
    buttonRect[4] = csharp;
    buttonRect[5] = d;
    buttonRect[6] = dsharp;
    buttonRect[7] = e;
    buttonRect[8] = f;
    buttonRect[9] = fsharp;
    buttonRect[10] = g;
    buttonRect[11] = gsharp;
//wavetype=(TextView) findViewById(R.id.wavetype);
    final TextView oct = findViewById(R.id.oct);
    oct.setText("-2");
    Button down = findViewById(R.id.octdown);
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
    Button up = findViewById(R.id.octup);
    up.setOnClickListener(new View.OnClickListener() {
        @Override
        public void onClick(View view) {
            if (octint < 0) {
                octint++;
                Integer intd = new Integer(octint);
                oct.setText(intd.toString());
            }
        }
    });
    Knob attack = findViewById(R.id.attack);
    attack.setState(50);
    attack.setOnStateChanged(new Knob.OnStateChanged() {
        @Override
        public void onState(int state) {
            // do something
            changeKnob(0, state);
        }
    });
    Knob decay = findViewById(R.id.decay);
    decay.setState(0);
    decay.setOnStateChanged(new Knob.OnStateChanged() {
        @Override
        public void onState(int state) {
            // do something
            changeKnob(1, state);
        }
    });
    Knob sustain = findViewById(R.id.sustain);
    sustain.setState(0);
    sustain.setOnStateChanged(new Knob.OnStateChanged() {
        @Override
        public void onState(int state) {
            // do something
            changeKnob(2, state);
        }
    });
    Knob release = findViewById(R.id.release);
    release.setState(0);
    release.setOnStateChanged(new Knob.OnStateChanged() {
        @Override
        public void onState(int state) {
            changeKnob(3, state);
            // do something
        }
    });
    filter = findViewById(R.id.filter);
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
    SeekBar q = findViewById(R.id.Q);
    changeKnob(5, 20);
    q.setProgress(20);
    q.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
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
    Knob drywet = findViewById(R.id.drywet);
    drywet.setState(0);
    drywet.setOnStateChanged(new Knob.OnStateChanged() {
        @Override
        public void onState(int state) {
            // do something
            changeKnob(6, state);
        }
    });
    Knob time = findViewById(R.id.delaytime);
    time.setState(0);
    time.setOnStateChanged(new Knob.OnStateChanged() {
        @Override
        public void onState(int state) {
            // do something
            changeKnob(7, state);
        }
    });
    Knob delayratio = findViewById(R.id.delayratio);
    delayratio.setState(0);
    delayratio.setOnStateChanged(new Knob.OnStateChanged() {
        @Override
        public void onState(int state) {
            // do something
            changeKnob(8, state);
        }
    });
    Knob feedback = findViewById(R.id.feedback);
    feedback.setState(0);
    feedback.setOnStateChanged(new Knob.OnStateChanged() {
        @Override
        public void onState(int state) {
            // do something
            changeKnob(9, state);
        }
    });
    SeekBar ratelfo = findViewById(R.id.LFORate);

    changeKnob(10, (int) ((300)));
    ratelfo.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
        @Override
        public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            changeKnob(10, (int) ((progress + 100) / 1.3));
        }

        @Override
        public void onStartTrackingTouch(SeekBar seekBar) {

        }

        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {

        }
    });
    SeekBar reverb = findViewById(R.id.reverb);
    reverb.setProgress(20);
    changeKnob(14, 20);
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

    Knob lfopitch = findViewById(R.id.LFOPitch);
    lfopitch.setState(60);
    lfopitch.setValueByAngle(5, false);
    changeKnob(12, lfopitch.getState());
    lfopitch.setOnStateChanged(new Knob.OnStateChanged() {
        @Override
        public void onState(int state) {
            // do something
            changeKnob(11, state);
        }
    });

    Knob lfofilter = findViewById(R.id.LFOFilter);
    lfofilter.setState(5);

    changeKnob(12, lfofilter.getState());
    lfofilter.setOnStateChanged(new Knob.OnStateChanged() {
        @Override
        public void onState(int state) {
            // do something
            changeKnob(12, state);
        }
    });
   drone = findViewById(R.id.drone);
    if(dronebool==false)
    changeKnob(13, 0);
    drone.setOnClickListener(new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            if (dronebool) {
                dronebool = false;
                changeKnob(13, 0);
               changeKnob(16,0);

            } else {
                dronebool = true;
                changeKnob(13, 1);

            }
        }
    });
    final Button selectwave = findViewById(R.id.selectWave);
    currentWave = 1;
    selectwave.setText(waves[currentWave]);
    changeKnob(15, currentWave);
    selectwave.setOnClickListener(new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            currentWave = (currentWave + 1) % waves.length;
            selectwave.setText(waves[currentWave]);
            changeKnob(15, currentWave);

        }
    });
    guide = findViewById(R.id.guide);
    guide.setMovementMethod(new ScrollingMovementMethod());
    guide.setText(guidetxt[0]);
    Button next = findViewById(R.id.next);
    Button prev = findViewById(R.id.prev);
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
            if(currentins>0)
            {
                currentins--;
            }
            displayinst();
        }
    });
    final EditText hertz=(EditText)findViewById(R.id.hertz);
    final ImageView tube=(ImageView)findViewById(R.id.tube);
    tube.setOnClickListener(new View.OnClickListener() {
        @Override
        public void onClick(View view) {

            ImageView tube=(ImageView) view;
            if (!tubeoff) {
                tube.setColorFilter(Color.rgb(10, 200, 150), PorterDuff.Mode.SRC_ATOP);
                tube.setAlpha((float)0.2);
                native_setTube(1);
                tubeoff=true;
            }    else {
                tubeoff=false;
                tube.clearColorFilter();
                tube.setAlpha((float)1);
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
                changeKnobDouble(16,test);


            } catch(Exception e) {
                Toast toast = Toast.makeText(getApplicationContext(), "Not a number", Toast.LENGTH_SHORT); toast.show();
                //EditText.setValue();
            }
        }
    });
}

    @Override
    protected void onStop() {
        super.onStop();
       if(dronebool==true) {
           dronebool = false;

           drone.toggle();
           changeKnob(13, 0);
           changeKnob(16, 0);
       }
    }



    @Override
    protected void onRestart() {
        super.onRestart();


    }
    static {
        System.loadLibrary("native-lib");
    }
    static boolean create(Context context){

        if (mEngineHandle == 0){
            setDefaultStreamValues(context);
            mEngineHandle = native_createEngine();
        }
        return (mEngineHandle != 0);
    }
ToggleButton drone;
    private static void setDefaultStreamValues(Context context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1){
            AudioManager myAudioMgr = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
            String sampleRateStr = myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
            int defaultSampleRate = Integer.parseInt(sampleRateStr);
            String framesPerBurstStr = myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
            int defaultFramesPerBurst = Integer.parseInt(framesPerBurstStr);

            native_setDefaultSampleRate(defaultSampleRate);
            native_setDefaultFramesPerBurst(defaultFramesPerBurst);
        }
    }
    private int octint=-2;
    private int currentins=0;
    private boolean dronebool = false;
    private boolean tubeon=false;
    private static final int MAXINST=4;
    public TextView guide;
    private String[] guidetxt = {"Welcome to Didgeridoo Drone a simple synth to jam along with or record\n Click next to continue reading instructions for use.", "Attack is the time for the note to get to max volume\nDecay is time from max attack to sustain level\nSustain is level when not is held for a long period\nRelease is how long for volume to go down as it is released. ",

"LFO rate is the speed of the low frequency wave that can affect the pitch or the filter\nLFO Filt is how much it affects the filter\nLFO pitch how much it affects oscillator\n",

            "Delay and reverb sound good with the drone experiment \nFor delay turn up the delay time, delay wet dry to about 50%, \nratio is the panning of the delay effect\n, as well as feedback, \nFor reverb turn up the wet dry below this text.",

        "To play Drone press drone button then select your note.\nTo go to a different octave select the button for it.",
        "To do a sine wave FM synthesis using one of the 3 oscillators as a modifier for the other frequencies but detuned slightly press the button with the wave name until it says FM sine.\n" +
                "when it says FM Sine turn up the lfo pitch rate until you get the desired effect from it, you cannot use the lfo to control pitch while it is performing FM synthesis on the sine waves" +
                "to go back to regular synth mode press the oscillator waveform button again",
            "Oscillator types, There is Saw Square and Triangle aswell as FM sinusoid. The sinusoid is a pure tone without hamonies, square and saw have alot of harmonies , triangle has less but its not just one like the sine wave so its a softer tone.",


};
    private static native long native_createEngine();
    private static native void native_deleteEngine(long engineHandle);
    private static native void native_setDefaultSampleRate(int sampleRate);
    private static native void native_setTube(int tube);
    private static native void native_setDefaultFramesPerBurst(int framesPerBurst);
    private native void touchEvent(int action, int key);

    private native void startEngine();

    private native void changeKnob(int knob, int val);private native void changeKnobDouble(int knob,double val);

    private TextView mLatencyText;

    private native void stopEngine();
    private int mSlop;
    private float mDownX;
    private float mDownY;
    private boolean mSwiping;
    private boolean tubeoff;
    private SeekBar filter;
    private String[] waves={"Harmonic", "Drone"};
    private int currentWave=0;
    View.OnTouchListener motionListener=new View.OnTouchListener() {
        boolean isPlaying=false;
        @Override
        public boolean onTouch(View v, MotionEvent event) {
            switch(event.getActionMasked()) {
                case ACTION_DOWN:
                    if(!isPlaying)
                        doKeys(0,v);
                    break;
                case ACTION_UP:
                    doKeys(2,v);
                    break;

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
double hertz=440;
    private TextView wavetype;
private Button[] buttonRect;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getSupportActionBar().hide(); //<< this
        setContentView(R.layout.activity_main);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

createStuff();
        RateThisApp.onCreate(this);
        RateThisApp.Config config = new RateThisApp.Config(2, 3);
        RateThisApp.init(config);
        RateThisApp.showRateDialogIfNeeded(this);

    }

//    @Override
//    public boolean onTouchEvent(MotionEvent event) {
//        touchEvent(event.getAction());
//        return super.onTouchEvent(event);
//    }


    @Override
    protected void onDestroy() {

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
    private boolean checkInterSection(Button view, float rawX, float rawY) {

//                       // View t=findViewById(R.id.content);
                       Rect slide= new Rect(view.getLeft(),view.getTop()+view.getHeight()/2,view.getRight(),view.getBottom()+view.getHeight()/2);
        return slide.contains((int) rawX, (int) rawY);
    }
    Button buttonlast;
    boolean noteon=false;
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
//                for (int i = 0; i < buttonRect.length; i++) {
//                    if (
//                            checkInterSection(buttonRect[i], event.getRawX(), event.getRawY())
//                    ) {
//
//                        if(buttonlast==null ||buttonRect[i].getId()!=buttonlast.getId()) {
//                           // Log.d("intersection","" +i);
//                            Log.d("intersection","" +i);
//                            buttonlast = buttonRect[i];
//                            doKeys(0,buttonlast);
//                            noteon=true;
//
//                        } else {
//                            if(noteon!=true) {
//                                doKeys(0, buttonRect[i]);
//                                buttonlast = buttonRect[i];
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
//                buttonlast=null;
//                noteon=false;
//                return false;
//
//
////            if ((rect!=null && !rect.contains(v.getLeft() + (int) event.getX(), v.getTop() + (int) event.getY()) )|| rect==null) {
////                // User moved outside bounds
////
////                boolean found=false;
////                    for(int i=0;i<12;i++) {
////                        View p=buttonRect[i];
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
//        buttonlast=null;
//        return false;
//
//    }


    public void doKeys(int event,View v) {
        switch (v.getId()) {

            case R.id.a:
                touchEvent(event, 12 * octint + 69);
                break;
            case R.id.b:
                touchEvent(event, 12 * octint + 71);
                break;
            case R.id.c:
                touchEvent(event, 12 * octint + 60);
                break;
            case R.id.d:
                touchEvent(event, 12 * octint + 62);
                break;
            case R.id.e:
                touchEvent(event, 12 * octint + 64);
                break;
            case R.id.f:
                touchEvent(event, 12 * octint + 65);
                break;
            case R.id.g:
                touchEvent(event, 12 * octint + 67);
                break;
            case R.id.asharp:
                touchEvent(event, 12 * octint + 70);
                break;
            case R.id.csharp:
                touchEvent(event, 12 * octint + 61);
                break;
            case R.id.dsharp:
                touchEvent(event, 12 * octint + 63);
                break;
            case R.id.fsharp:
                touchEvent(event, 12 * octint + 66);
                break;
            case R.id.gsharp:
                touchEvent(event, 12 * octint + 68);
                break;
//                default:
//                    touchEvent(2, 12 * octint + 68);

        }
    }
//    public void doKeys(MotionEvent event,View v) {
//        switch (v.getId()) {
//
//            case R.id.a:
//                touchEvent(event.getActionMasked(), 12 * octint + 69);
//                break;
//            case R.id.b:
//                touchEvent(event.getActionMasked(), 12 * octint + 71);
//                break;
//            case R.id.c:
//                touchEvent(event.getActionMasked(), 12 * octint + 60);
//                break;
//            case R.id.d:
//                touchEvent(event.getActionMasked(), 12 * octint + 62);
//                break;
//            case R.id.e:
//                touchEvent(event.getActionMasked(), 12 * octint + 64);
//                break;
//            case R.id.f:
//                touchEvent(event.getActionMasked(), 12 * octint + 65);
//                break;
//            case R.id.g:
//                touchEvent(event.getActionMasked(), 12 * octint + 67);
//                break;
//            case R.id.asharp:
//                touchEvent(event.getActionMasked(), 12 * octint + 70);
//                break;
//            case R.id.csharp:
//                touchEvent(event.getActionMasked(), 12 * octint + 61);
//                break;
//            case R.id.dsharp:
//                touchEvent(event.getActionMasked(), 12 * octint + 63);
//                break;
//            case R.id.fsharp:
//                touchEvent(event.getActionMasked(), 12 * octint + 66);
//                break;
//            case R.id.gsharp:
//                touchEvent(event.getActionMasked(), 12 * octint + 68);
//                break;
////                default:
////                    touchEvent(2, 12 * octint + 68);
//
//    }
}
