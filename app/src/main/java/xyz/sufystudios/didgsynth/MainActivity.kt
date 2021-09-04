package xyz.sufystudios.didgsynth

import androidx.appcompat.app.AppCompatActivity
import com.anjlab.android.iab.v3.BillingProcessor.IBillingHandler
import xyz.sufystudios.didgsynth.MainActivity
import android.os.Build
import android.media.AudioManager
import com.anjlab.android.iab.v3.BillingProcessor
import com.google.android.material.button.MaterialButton
import android.content.Intent
import xyz.sufystudios.didgsynth.R.id
import it.beppi.knoblibrary.Knob
import it.beppi.knoblibrary.Knob.OnStateChanged
import android.widget.SeekBar.OnSeekBarChangeListener
import android.annotation.SuppressLint
import android.text.method.ScrollingMovementMethod
import android.graphics.PorterDuff
import android.text.TextWatcher
import android.text.Editable
import android.view.View.OnTouchListener
import android.view.MotionEvent
import xyz.sufystudios.didgsynth.PlaybackEngine
import android.app.Activity
import android.content.Context
import android.graphics.Color
import android.graphics.Rect
import android.os.Bundle
import android.util.Log
import android.view.View
import xyz.sufystudios.didgsynth.R.layout
import android.view.WindowManager
import android.widget.*
import com.anjlab.android.iab.v3.TransactionDetails
import com.google.android.material.slider.Slider
import com.google.android.material.textview.MaterialTextView
import java.lang.Exception



//import com.kobakei.ratethisapp.RateThisApp;
//import xyz.murdoch.edu.au.learnsynth.R;
class MainActivity : AppCompatActivity(), IBillingHandler {

    companion object {
        var mEngineHandle: Long = 0
        fun create(context: Context): Boolean {
            if (mEngineHandle == 0L) {
                setDefaultStreamValues(context)
            }
            return mEngineHandle != 0L
        }

        private fun setDefaultStreamValues(context: Context) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
                val myAudioMgr = context.getSystemService(AUDIO_SERVICE) as AudioManager
                val sampleRateStr = myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE)
                val defaultSampleRate = sampleRateStr.toInt()
                val framesPerBurstStr = myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER)
                val defaultFramesPerBurst = framesPerBurstStr.toInt()
            }
        }

        private const val MAXINST = 4
        private external fun native_setTube(tube: Int)

        // Load native library
        init {
            System.loadLibrary("native-lib")
        }
    }

    var context: Context? = null
    private var bp: BillingProcessor? = null
    var aiDroneString = "ai_drone_upgrade"
    var AI_PURHCASED = false
    var AION = false
    var aiDrone: MaterialButton? = null


    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        if (!bp!!.handleActivityResult(requestCode, resultCode, data)) {
            super.onActivityResult(requestCode, resultCode, data)
        }
    }

    protected fun toggleAIDrone() {
        val products = bp!!.listOwnedProducts()
        for (i in products) {
            if (i == aiDroneString) {
                AI_PURHCASED = true
            }
        }
        if (!AI_PURHCASED) {
            bp!!.purchase(this, aiDroneString)
        }
        if (AI_PURHCASED && !AION) {
            aiDrone!!.text = "AI:ON"
            AION = true
            changeKnob(21, 1)
        } else {
            aiDrone!!.text = "AI:OFF"
            AION = false
            changeKnob(21, 0)
        }
    }

    private fun createStuff() {
        aiDrone = findViewById<MaterialButton>(id.aibutton)
        //startEngine();
        bp = BillingProcessor(this, "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAp60Ho4VkC75TR1HXwwbYK/1MIMP2vhE8S6LyHIY6WdAWuThV8czkvzlGv3HaExH1bbVUl3k5jKYvS+JLIyuHCY7bSFLKs/nQTI8ZynbEElvmnVL7sW84LlRLA32/K1u1YNligjY6b8m20hroZZNuSavaHpn6foSfrLaIXMIKBLCQEqmhd+V5Y3xSVCF2/5b0ACPyG95GYcquEuX5N93/42i5xVb3ZotBrclwQf/9tIfMne3BDcWJFS7rQOlpNM0+QrW1Q9dTUP3bLVkEHRbtyhPUSBa5O03c0lmil+I/86TnO24Y5d0+A64EWnvIiP/+AtA2Ud62htGOhKVLWtK6cQIDAQAB", this)
        bp!!.initialize()
        var a = findViewById<MaterialButton>(id.a)
        a.setOnTouchListener(motionListener)

        aiDrone?.text = "AI:OFF"
        aiDrone?.setOnClickListener { toggleAIDrone() }
        var asharp = findViewById<MaterialButton>(id.asharp)
        asharp.setOnTouchListener(motionListener)
        var b = findViewById<MaterialButton>(id.b)
        b.setOnTouchListener(motionListener)
        var c = findViewById<MaterialButton>(id.c)
        c.setOnTouchListener(motionListener)
        var csharp = findViewById<MaterialButton>(id.csharp)
        csharp.setOnTouchListener(motionListener)
        var d = findViewById<MaterialButton>(id.d)
        d.setOnTouchListener(motionListener)
        var dsharp = findViewById<MaterialButton>(id.dsharp)
        dsharp.setOnTouchListener(motionListener)
        var e = findViewById<MaterialButton>(id.e)
        e.setOnTouchListener(motionListener)
        var f = findViewById<MaterialButton>(id.f)
        f.setOnTouchListener(motionListener)
        var fsharp = findViewById<MaterialButton>(id.fsharp)
        fsharp.setOnTouchListener(motionListener)
        var g = findViewById<MaterialButton>(id.g)
        g.setOnTouchListener(motionListener)
        var gsharp = findViewById<MaterialButton>(id.gsharp)
        gsharp.setOnTouchListener(motionListener)

        val oct = findViewById<TextView>(id.oct)
        oct.text = "0"
        val down = findViewById<MaterialButton>(id.octdown)
        down.setOnClickListener {
            if (octint > -3) {
                octint--
                val intd = octint
                oct.text = intd.toString()
            }
        }
        val up = findViewById<MaterialButton>(id.octup)
        up.setOnClickListener {
            if (octint < 3) {
                octint++
                val intd = octint
                oct.text = intd.toString()
            }
        }
        val attack = findViewById<Knob>(id.attack)
        attack.state = 50
        attack.setOnStateChanged { state -> // do something
            changeKnob(0, state)
        }
        val decay = findViewById<Knob>(id.decay)
        decay.state = 0
        decay.setOnStateChanged { state -> // do something
            changeKnob(1, state)
        }
        val sustain = findViewById<Knob>(id.sustain)
        sustain.state = 0
        sustain.setOnStateChanged { state -> // do something
            changeKnob(2, state)
        }
        val release = findViewById<Knob>(id.release)
        release.state = 0
        release.setOnStateChanged { state ->
            changeKnob(3, state)
            // do something
        }
        val filter = findViewById<SeekBar>(id.filter)
       
        changeKnob(4, 100)
        filter.run {
            release.state = 0
            release.setOnStateChanged { state ->
            changeKnob(3, state)
            // do something
        }

            progress = 100
            changeKnob(4, 100)
            setOnSeekBarChangeListener(object : OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar, i: Int, b: Boolean) {
                changeKnob(4, i)
            }

            override fun onStartTrackingTouch(seekBar: SeekBar) {}
            override fun onStopTrackingTouch(seekBar: SeekBar) {}
        })
        }
        val Q = findViewById<SeekBar>(id.Q)
        changeKnob(5, 20)
        Q.progress = 20
        Q.setOnSeekBarChangeListener(object : OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar, i: Int, b: Boolean) {
                changeKnob(5, i)
            }

            override fun onStartTrackingTouch(seekBar: SeekBar) {}
            override fun onStopTrackingTouch(seekBar: SeekBar) {}
        })
        val q = findViewById<SeekBar>(id.q)
        changeKnob(19, 23)
        q.progress = 23
        q.setOnSeekBarChangeListener(object : OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar, i: Int, b: Boolean) {
                changeKnob(19, i)
            }

            override fun onStartTrackingTouch(seekBar: SeekBar) {}
            override fun onStopTrackingTouch(seekBar: SeekBar) {}
        })
        val drywet = findViewById<Knob>(id.drywet)
        drywet.state = 20
        changeKnob(6, 20)
        drywet.setOnStateChanged { state -> // do something
            changeKnob(6, state)
        }
        val time = findViewById<Knob>(id.delaytime)
        time.state = 49
        changeKnob(7, 49)
        time.setOnStateChanged { state -> // do something
            changeKnob(7, state)
        }
        val delayratio = findViewById<Knob>(id.delayratio)
        delayratio.state = 18
        changeKnob(8, 18)
        delayratio.setOnStateChanged { state -> // do something
            changeKnob(8, state)
        }
        val feedback = findViewById<Knob>(id.feedback)
        feedback.state = 20
        changeKnob(9, 20)
        feedback.setOnStateChanged { state -> // do something
            changeKnob(9, state)
        }
        val ratelfo = findViewById<SeekBar>(id.LFORate)
        ratelfo.progress = 70
        changeKnob(10, 50)
        ratelfo.setOnSeekBarChangeListener(object : OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                changeKnob(10, progress)
            }

            override fun onStartTrackingTouch(seekBar: SeekBar) {}
            override fun onStopTrackingTouch(seekBar: SeekBar) {}
        })
        val reverb = findViewById<SeekBar>(id.reverb)
        reverb.progress = 20
        changeKnob(14, 20)
        reverb.setOnSeekBarChangeListener(object : OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                changeKnob(14, progress)
            }

            override fun onStartTrackingTouch(seekBar: SeekBar) {}
            override fun onStopTrackingTouch(seekBar: SeekBar) {}
        })
        val randomlfo = findViewById<MaterialButton>(id.randlfo)
        randomlfo.setBackgroundColor(Color.RED)
        randomlfo.setOnClickListener(object : View.OnClickListener {
            private var randlfo = false
            @SuppressLint("ResourceAsColor", "ResourceType")
            override fun onClick(view: View) {
                randlfo = if (!randlfo) {
                    randomlfo.setBackgroundColor(Color.GREEN)
                    changeKnob(17, 1)
                    true
                } else {
                    randomlfo.setBackgroundColor(Color.RED)
                    changeKnob(17, 0)
                    false
                }
            }
        })
        val randomovertones = findViewById<MaterialButton>(id.randomovertones)
        randomovertones.setBackgroundColor(Color.RED)
        randomovertones.setOnClickListener(object : View.OnClickListener {
            private var randover = false
            @SuppressLint("ResourceAsColor", "ResourceType")
            override fun onClick(view: View) {
                randover = if (!randover) {
                    randomovertones.setBackgroundColor(Color.GREEN)
                    changeKnob(18, 1)
                    true
                } else {
                    randomovertones.setBackgroundColor(Color.RED)
                    changeKnob(18, 0)
                    false
                }
            }
        })
        val looper = findViewById<MaterialButton>(id.looper)
        looper.setBackgroundColor(Color.RED)
        looper.setOnClickListener(object : View.OnClickListener {
            private var loop = false
            @SuppressLint("ResourceAsColor", "ResourceType")
            override fun onClick(view: View) {
                loop = if (!loop) {
                    looper.setBackgroundColor(Color.GREEN)
                    changeKnob(20, 1)
                    true
                } else {
                    looper.setBackgroundColor(Color.RED)
                    changeKnob(20, 0)
                    false
                }
            }
        })
        val lfopitch = findViewById<Knob>(id.LFOPitch)
        lfopitch.state = 2
        changeKnob(11, 2)
        lfopitch.setOnStateChanged { state -> // do something
            changeKnob(11, state)
        }
        val lfofilter = findViewById<Knob>(id.LFOFilter)
        lfofilter.state = 5
        changeKnob(12, 5)
        lfofilter.setOnStateChanged { state -> // do something
            changeKnob(12, state)
        }
        val drone = findViewById<View>(id.drone) as MaterialButton
        if (dronebool == false) changeKnob(13, 0)
        drone.run {
            lfofilter.state = 5
            changeKnob(12, 5)
            lfofilter.setOnStateChanged { state -> // do something
            changeKnob(12, state)
        }

            if (dronebool == false) changeKnob(13, 0)
            setOnClickListener(View.OnClickListener {
            if (dronebool) {
                droneint = 0
                dronebool = false
                changeKnob(13, 0)
                changeKnob(16, 0)
                run { text = "Drone OFF" }
            } else {
                droneint = 1
                dronebool = true
                changeKnob(13, 1)
                text = "Drone ON"
            }
        })
        }
        val selectwave = findViewById<MaterialButton>(id.selectWave)
        selectwave.setOnClickListener {
            currentWave = (currentWave + 1) % waves.size
            selectwave.text = waves[currentWave]
            changeKnob(15, currentWave)
        }
        val guide = findViewById<MaterialTextView>(id.guide)
        guide.run {
            selectwave.setOnClickListener {
            currentWave = (currentWave + 1) % waves.size
            selectwave.text = waves[currentWave]
            changeKnob(15, currentWave)
        }

            movementMethod = ScrollingMovementMethod()
            text = guidetxt[0]
        }
        val next = findViewById<MaterialButton>(id.next)
        val prev = findViewById<MaterialButton>(id.prev)
        val hide = findViewById<MaterialButton>(id.hidehelp)
        hide.setOnClickListener(object : View.OnClickListener {
            var helphidden = false
            override fun onClick(view: View) {
                if (!helphidden) {
                    guide.visibility = View.GONE
                    next.visibility = View.GONE
                    prev.visibility = View.GONE
                    helphidden = true
                } else {
                    guide.visibility = View.VISIBLE
                    next.visibility = View.VISIBLE
                    prev.visibility = View.VISIBLE
                    helphidden = false
                }
            }
        })
        next.setOnClickListener {
            if (currentins < guidetxt.size - 1) {
                currentins++
            }
            displayinst()
        }
        prev.setOnClickListener {
            if (currentins > 0) {
                currentins--
            }
            displayinst()
        }
        val hertz = findViewById<View>(id.hertz) as EditText
        val tube = findViewById<View>(id.tube) as ImageView
        tube.setOnClickListener { view ->
            val tube = view as ImageView
            if (!tubeoff) {
                tube.setColorFilter(Color.rgb(10, 200, 150), PorterDuff.Mode.SRC_ATOP)
                tube.alpha = 0.2.toFloat()
                native_setTube(1)
                tubeoff = true
            } else {
                tubeoff = false
                tube.clearColorFilter()
                tube.alpha = 1.toFloat()
                native_setTube(1)
            }
        }
        hertz.addTextChangedListener(object : TextWatcher {
            override fun beforeTextChanged(s: CharSequence, start: Int, count: Int, after: Int) {}
            override fun onTextChanged(s: CharSequence, start: Int, before: Int, count: Int) {}
            override fun afterTextChanged(s: Editable) {
                try {
                    val test = hertz.text.toString().toDouble()
                    changeKnobDouble(16, test)
                } catch (e: Exception) {
                    val toast = Toast.makeText(applicationContext, "Not a number", Toast.LENGTH_SHORT)
                    toast.show()
                    //EditText.setValue();
                }
            }
        })
    }

    private var octint = 0
    private var currentins = 0
    private var dronebool = false
    private val tubeon = false
    var guide: TextView? = null
    private val guidetxt = arrayOf("Welcome to Didg Synth, a simple synth to jam along with or record using an audio interface\n Click next to continue reading instructions for use.", "Attack is the time for the note to get to max volume\nDecay is time from max attack to sustain level\nSustain is level when the note is held for a long period\nRelease is how long for volume to go down as it is released. ",
            "LFO rate is the speed of the low frequency wave that can affect the pitch or the filter\nLFO Filt is how much it affects the filter\nLFO pitch how much it affects oscillator\n",
            "Delay and reverb sound good with the drone experiment \nFor delay turn up the delay time, delay wet dry to about 50%, \nratio is the time for each channel either left or right, you can ballance them or make the delay higher in one channel\n, as well as feedback, \nFor reverb turn up the wet dry below this text.",
            "To play Drone press drone MaterialButton  then select your note.\nTo go to a different octave select the MaterialButton  for it.",
            """
                To do a sine wave FM synthesis using one of the 3 oscillators as a modifier for the other frequencies but detuned slightly press the MaterialButton  with the wave name until it says FM sine.
                to go back to regular synth mode press the oscillator waveform MaterialButton  again
                """.trimIndent(),
            "Oscillator types, There is Saw Square and Triangle aswell as FM sinusoid. The sinusoid is a pure tone without hamonies, square and saw have alot of harmonies , Noise can sound like ocean waves or wind, triangle has less but its not just one like the sine wave so its a softer tone.")

    private external fun touchEvent(action: Int, key: Int)
    private external fun changeKnob(knob: Int, `val`: Int)
    private external fun changeKnobDouble(knob: Int, `val`: Double)
    private val mLatencyText: TextView? = null
    private val mSlop = 0
    private val mDownX = 0f
    private val mDownY = 0f
    private val mSwiping = false
    private var tubeoff = false
    private var filter: SeekBar? = null
    private val waves = arrayOf("Harmony", "Drone")
    private var currentWave = 0
    private val lastview: View? = null
    var motionListener: OnTouchListener = object : OnTouchListener {
        var isplaying = false
        override fun onTouch(v: View, event: MotionEvent): Boolean {
            when (event.actionMasked) {
                MotionEvent.ACTION_DOWN -> {
                    doKeys(0, v)
                    return true
                }
                MotionEvent.ACTION_UP -> {
                    doKeys(2, v)
                    // doKeys(2, v);
                    return true
                }
            }
            return false
        }
    }

    //private class TouchListener implements View.OnClickListener {
    //    @Override
    //    public void onClick(View view) {
    //        doKeys;
    //    }
    //}
    var hertz = 440.0
    private val wavetype: TextView? = null

    //    private MaterialButton [] MaterialButton Rect;
    private var drone: MaterialButton? = null
    override fun onPause() {
        PlaybackEngine.stop()
        super.onPause()
    }

    override fun onResume() {
        PlaybackEngine.start()
        Log.d("on resume", "on resume")
        super.onResume()
        //   createStuff();
    }

    var thisactivity: Activity? = null
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        context = this.applicationContext
        //getSupportActionBar().hide(); //<< this
        thisactivity = this
        setContentView(layout.activity_main)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        PlaybackEngine.create(this)
        createStuff()


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
    override fun onDestroy() {
        if (bp != null) {
            bp!!.release()
        }
        PlaybackEngine.delete()
        super.onDestroy()
    }

    private fun displayinst() {
        guide!!.text = guidetxt[currentins]
    }

    fun nextinst() {}
    fun previnst() {}
    private val rect // Variable rect to hold the bounds of the view
            : Rect? = null

    private fun checkInterSection(view: MaterialButton, rawX: Float, rawY: Float): Boolean {

//                       // View t=findViewById(R.id.content);
        val slide = Rect(view.left, view.top + view.height / 2, view.right, view.bottom + view.height / 2)
        return slide.contains(rawX.toInt(), rawY.toInt())
    }

    var last: MaterialButton? = null
    var noteon = false

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
    var droneint = 0
    fun doKeys(event: Int, v: View) {
        when (v.id) {
            id.a -> touchEvent(event + droneint, 12 * octint + 69)
            id.b -> touchEvent(event + droneint, 12 * octint + 71)
            id.c -> touchEvent(event + droneint, 12 * octint + 60)
            id.d -> touchEvent(event + droneint, 12 * octint + 62)
            id.e -> touchEvent(event + droneint, 12 * octint + 64)
            id.f -> touchEvent(event + droneint, 12 * octint + 65)
            id.g -> touchEvent(event + droneint, 12 * octint + 67)
            id.asharp -> touchEvent(event + droneint, 12 * octint + 70)
            id.csharp -> touchEvent(event + droneint, 12 * octint + 61)
            id.dsharp -> touchEvent(event + droneint, 12 * octint + 63)
            id.fsharp -> touchEvent(event + droneint, 12 * octint + 66)
            id.gsharp -> touchEvent(event + droneint, 12 * octint + 68)
        }
    }

    fun doKeys(event: MotionEvent, v: View) {
        when (v.id) {
            id.a -> touchEvent(event.actionMasked, 12 * octint + 69)
            id.b -> touchEvent(event.actionMasked, 12 * octint + 71)
            id.c -> touchEvent(event.actionMasked, 12 * octint + 60)
            id.d -> touchEvent(event.actionMasked, 12 * octint + 62)
            id.e -> touchEvent(event.actionMasked, 12 * octint + 64)
            id.f -> touchEvent(event.actionMasked, 12 * octint + 65)
            id.g -> touchEvent(event.actionMasked, 12 * octint + 67)
            id.asharp -> touchEvent(event.actionMasked, 12 * octint + 70)
            id.csharp -> touchEvent(event.actionMasked, 12 * octint + 61)
            id.dsharp -> touchEvent(event.actionMasked, 12 * octint + 63)
            id.fsharp -> touchEvent(event.actionMasked, 12 * octint + 66)
            id.gsharp -> touchEvent(event.actionMasked, 12 * octint + 68)
        }
    }

    var crect: Rect? = null
    var drect: Rect? = null
    var erect: Rect? = null
    var frect: Rect? = null
    var grect: Rect? = null
    var arect: Rect? = null
    var brect: Rect? = null
    var csharprect: Rect? = null
    var dsharprect: Rect? = null
    var fsharprect: Rect? = null
    var gsharprect: Rect? = null
    var asharprect: Rect? = null
    var isdown = false
    override fun onProductPurchased(productId: String, details: TransactionDetails?) {
        if (productId == aiDroneString) {
            AI_PURHCASED = true
            toggleAIDrone()
        }
    }

    override fun onPurchaseHistoryRestored() {}
    override fun onBillingError(errorCode: Int, error: Throwable?) {}
    override fun onBillingInitialized() {} /*
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