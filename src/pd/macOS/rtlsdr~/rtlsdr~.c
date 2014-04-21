/**
 @file
 rtlsdr~ - a very simple example of a basic MSP object
 
 updated 3/22/09 ajm: new API
 
 @ingroup	examples
 */

#define PDVERSION    // uncomment this for Pd

// tz this is the Max version from 3/28/2014 brought over to Pd land to adapt it


#ifdef PDVERSION
#include "m_pd.h"
t_class *rtlsdr_class;
#else
#include "ext.h"
#include "ext_obex.h"
#include "z_dsp.h"
void *rtlsdr_class;
#endif

#include <pthread.h>	// for threading in rtl_fm
#include <string.h>
// #include <ctype.h>
#include <stdlib.h>


#define CIRCMAX 1500000 // this should be in an include file

extern int16_t circ_buf_left[CIRCMAX * sizeof(int16_t) + 1];

extern int circ_index_write;
extern int circ_index_read;
extern int circ_buf_latency_factor;


int rtlsdr_already_loaded = 0;  // flag for singleton instance requirement


extern uint32_t new_freq;
extern int need_to_set_freq;

extern int new_gain;
extern int need_to_set_gain;

extern char new_mode[];
extern int need_to_set_mode;


extern int need_to_reset_circ_buffer;


// from rtl_fm


#define AUTO_GAIN			-100



//////////////////////////////
//
// object data structure
//

typedef struct _rtlsdr {
#ifdef PDVERSION
	t_object 	obj;
	float		x_f;		// for pd internal use
#else
    t_pxobject	x_obj;
#endif
    double		x_val;
	
    int         x_gain_db10;            // gain in tenths of db
    long        x_freq_hz;              // radio frequency setting
    int         x_radio_is_running;     // needed, because we can only run one radio at a time
    
    long        x_device_samplerate;        // device output sample rate (-s param)

    long        x_device_input_samplerate;         // device input sample rate (not used yet)
    int         x_downsample;           // multiple of device input SR to output SR (not used yet)

    long        x_sr;                   // current Max/Pd sample rate
    
} t_rtlsdr;

//////////////////////////////////
//
// function prototypes for this class
//

// input handlers -


// shared by Max and Pd

void rtlsdr_freq (t_rtlsdr *x, t_symbol *mesg, short argc, t_atom *argv );
void rtlsdr_gain (t_rtlsdr *x, t_symbol *mesg, short argc, t_atom *argv );
void rtlsdr_info (t_rtlsdr *x );
void rtlsdr_samplerate (t_rtlsdr *x, t_symbol *mesg, short argc, t_atom *argv);
void rtlsdr_resetbuf (t_rtlsdr *x);
void rtlsdr_stop (t_rtlsdr *x );
void rtlsdr_start (t_rtlsdr *x );


#ifdef PDVERSION	// pd only
void rtlsdr_tilde_setup (void);
void *rtlsdr_new( void );
void rtlsdr_dsp(t_rtlsdr *x, t_signal **sp, short *count);
t_int *rtlsdr_perform(t_int *w);
void rtlsdr_free( t_rtlsdr *x);


#else	// max only

void *rtlsdr_new(double val);
void rtlsdr_dsp64(t_rtlsdr *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void rtlsdr_perform64(t_rtlsdr *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);
void rtlsdr_free( t_rtlsdr *x);

// max assist
void rtlsdr_assist(t_rtlsdr *x, void *b, long m, long a, char *s);

// max input handlers
void rtlsdr_float(t_rtlsdr *x, double f);
void rtlsdr_int(t_rtlsdr *x, long n);

// void rtlsdr_bang(t_rtlsdr *x);

#endif


// misc prototypes

float linear_map( float val, float leftMin, float leftMax , float rightMin, float rightMax);

// prototypes from other files...

int stop_the_radio(void);
int start_the_radio(int device_index, long freq_hz, long output_SR, int gain_db10);

//////////////////////////////////////////
//
// class setup routine
//
#ifdef PDVERSION
void rtlsdr_tilde_setup (void)
{
	/* Initialize the class */
    
	
	rtlsdr_class = class_new(gensym("rtlsdr~"), (t_newmethod)rtlsdr_new, (t_method) rtlsdr_free, sizeof(t_rtlsdr), 0, 0);
    
	/* Specify signal input, with automatic float to signal conversion */
    
	CLASS_MAINSIGNALIN(rtlsdr_class, t_rtlsdr, x_f);
	
	// input methods (to receive messages)
    
	class_addmethod(rtlsdr_class, (t_method)rtlsdr_freq, gensym("freq"), A_GIMME, 0);
    class_addmethod(rtlsdr_class, (t_method)rtlsdr_gain, gensym("gain"), A_GIMME, 0);
	class_addmethod(rtlsdr_class, (t_method)rtlsdr_info, gensym("info"), 0);
    
	class_addmethod(rtlsdr_class, (t_method)rtlsdr_samplerate, gensym("samplerate"), A_GIMME, 0);
	class_addmethod(rtlsdr_class, (t_method)rtlsdr_resetbuf, gensym("resetbuf"), 0);
	class_addmethod(rtlsdr_class, (t_method)rtlsdr_stop, gensym("stop"), 0);
	class_addmethod(rtlsdr_class, (t_method)rtlsdr_start, gensym("start"), 0);
	
	/* Bind the DSP method, which is called when the DACs are turned on */
	
	class_addmethod(rtlsdr_class, (t_method)rtlsdr_dsp, gensym("dsp"),0);
	
	
	/* Print authorship message to the Max window */
	
	post("rtlsdr~ from \"rtlsdr~ external\" by Richard M. Nixon");

}
#else
int C74_EXPORT main(void)
{
    t_class *c = class_new("rtlsdr~", (method)rtlsdr_new, (method)rtlsdr_free, sizeof(t_rtlsdr), NULL, A_DEFFLOAT, 0);
    
    // this needs to be here even if we don't need to input any signals...
    class_addmethod(c, (method)rtlsdr_dsp64, "dsp64", A_CANT, 0); 	// respond to the dsp message
    
    class_addmethod(c, (method)rtlsdr_float, "float", A_FLOAT, 0);
    class_addmethod(c, (method)rtlsdr_int, "int", A_LONG, 0);
    
    class_addmethod(c, (method)rtlsdr_freq, "freq", A_GIMME, 0 );
    class_addmethod(c, (method)rtlsdr_gain, "gain", A_GIMME, 0 );
    class_addmethod(c, (method)rtlsdr_samplerate, "samplerate", A_GIMME, 0 );
    class_addmethod(c, (method)rtlsdr_resetbuf, "resetbuf", 0);
    class_addmethod(c, (method)rtlsdr_stop, "stop", 0);
    class_addmethod(c, (method)rtlsdr_start, "start", 0);
    class_addmethod(c, (method)rtlsdr_info, "info", 0);
    
    //    class_addmethod(c, (method)rtlsdr_bang,"bang", BANG, 0);
    
    // need to add message input here...
    
    class_addmethod(c, (method)rtlsdr_assist,"assist",A_CANT,0);
    
    class_dspinit(c);												// must call this function for MSP object classes
    
	class_register(CLASS_BOX, c);
	rtlsdr_class = c;
    
    
	return 0;
}
#endif

///////////////////////////
//
// new instance routine
//
#ifdef PDVERSION
void *rtlsdr_new( void )
{
	/* Instantiate a new rtlsdr~ object with one signal inlet */
	
    t_rtlsdr *x = (t_rtlsdr *) pd_new(rtlsdr_class);
	
	/* Create 2 signal outlets */
	
    outlet_new(&x->obj, gensym("signal"));
	outlet_new(&x->obj, gensym("signal"));
	
	// initialize data structure to default settings
	
	x->x_freq_hz = 162500000;   // default freq
    x->x_gain_db10 = AUTO_GAIN;      // default gain
    
    x->x_radio_is_running = 0;  // radio is off
    
    x->x_sr = (long) sys_getsr();  // sample rate
    x->x_device_samplerate = (long) sys_getsr();   // rtl device output SR
    
    
    post("rtlsdr~ loaded", 0);
	
    
	/* Return a pointer to the new object */
    
    return x;
}

#else   // Max version of new instance routine

void *rtlsdr_new(double val)
{
    if(rtlsdr_already_loaded) {
        post("an instance of rtlsdr is already loaded. Please use only one rtlsdr object", 0);
        return(NULL);
    }
    else {
        rtlsdr_already_loaded = 1;
    }
    
    t_rtlsdr *x = object_alloc(rtlsdr_class);
    // dsp_setup((t_pxobject *)x, 2);			// set up DSP for the instance and create signal inlets
    
    dsp_setup((t_pxobject *)x, 1);			// set up DSP for the instance and create signal inlet
    
    outlet_new((t_pxobject *)x, "signal");			// signal outlets are created like this
    
    outlet_new((t_pxobject *)x, "signal");			// signal outlets are created like this
    
    x->x_val = val;		// what is this? i forgot
    
    x->x_freq_hz = 162500000;   // default freq
    x->x_gain_db10 = AUTO_GAIN;      // default gain
    
    
    x->x_radio_is_running = 0;  // radio is off
    
    x->x_sr = (long) sys_getsr();  // sample rate
    x->x_device_samplerate = (long) sys_getsr();   // rtl device output sample rate
    
    post("rtlsdr~ loaded", 0);
    return (x);
}
#endif


//////////////////////////////////////////////
//
// DSP routine
//

#ifdef PDVERSION
void rtlsdr_dsp(t_rtlsdr *x, t_signal **sp, short *count)
{
	
	
    need_to_reset_circ_buffer = 1;  // causes reset of buffer indexes
    
    // respond to sample rate changes...
    //
    
    if(x->x_sr != sp[0]->s_sr) {
        x->x_device_samplerate = (long) sp[0]->s_sr;   // change device output SR
        x->x_sr = (long) sp[0]->s_sr;              // change Max/Pd stored SR
        
        if(x->x_radio_is_running) {
            // restart, using current settings
            rtlsdr_start( x );
        }
    }
	/* Call the dsp_add() function, passing the DSP routine to
	 be used, which is rtlsdr_perform() in this case; the number of remaining
	 arguments; a pointer to the signal inlet; a pointer to the signal outlet;
	 and finally, the signal vector size in samples.
	 */
	dsp_add(rtlsdr_perform, 5,x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

#else   // Max version
// method called when dsp is turned on
void rtlsdr_dsp64(t_rtlsdr *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    
    need_to_reset_circ_buffer = 1;  // causes reset of buffer indexes
    
    // respond to sample rate changes...
    //
    if(x->x_sr != samplerate) {
        x->x_device_samplerate = (long) samplerate;            // change device output SR
        x->x_sr = (long) samplerate;                       // change Max stored SR
        
        if(x->x_radio_is_running) {
            // restart, using current settings
            rtlsdr_start( x );
        }
    }
    
    object_method(dsp64, gensym("dsp_add64"), x, rtlsdr_perform64, 0, NULL);
    
}

#endif

////////////////////////////////
//
// "perform" routine
//
//
#ifdef PDVERSION
t_int *rtlsdr_perform(t_int *w)
{
	t_rtlsdr *x =  (t_rtlsdr *) w[1];
	
	/* Copy the signal inlet pointer */
    
	float *in = (t_float *) (w[2]);
    
	/* Copy the signal outlet pointer */
    
	float *out1 = (t_float *) (w[3]);
	
	/* Copy the signal outlet pointer for second outlet */
    
	float *out2 = (t_float *) (w[4]);
    
	/* Copy the signal vector size */
    
	int n = w[5];
	
	/////////////////
	
    int i;
    char mesg[100];
    int veclen = n;
    static int display_counter = 0;
    static int display_max = 150;    // how often to display debug info
    int diff;
    
    
    //  outlets will send IQ data
   
    
    i = 0;
    while (n--) {
        
        if(circ_index_read > CIRCMAX) {
            circ_index_read = 0;
        }
        
        // if radio is off, just output zero's
        if(x->x_radio_is_running == 0) {
            *out1 = 0;
            *out2 = 0;
            
        }
        
        // raw mode sends baseband IQ data - everything else is demodulated audio
        // note that raw mode reads buffer twice as fast because its interleaved IQ
        else  {
            *out1 = linear_map((float) circ_buf_left[circ_index_read++], -32768, 32767, -1, 1);
            if(circ_index_read > CIRCMAX) {
                circ_index_read = 0;
            }
            *out2 = linear_map((float) circ_buf_left[circ_index_read++], -32768, 32767, -1, 1);
            
        }
      
        
        out1++;
        out2++;
        
        
    }
    
    
	// debugging stuff for circular buffer and latency
    
    // display_counter--;   // uncomment this line to display debug info
    if(display_counter <= 0) {
        display_counter = display_max;
        if( circ_index_write > circ_index_read) {
            diff = circ_index_write - circ_index_read;
        }
        else {
            diff = (circ_index_write + CIRCMAX) - circ_index_read;
        }
        sprintf(mesg, "wndx %d:, rndx: %d, diff: %d veclen: %d", circ_index_write, circ_index_read, diff, veclen  );
        post(mesg, 0);
        
    }
	
	
	
	/////////////////
	
	
		/* Return the next address on the signal chain */
    
	return w + 6;
}

#else   // Max version of perform

// tz this is the one that actuall runs all the time...
void rtlsdr_perform64(t_rtlsdr *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    int i;
    char mesg[100];
    int veclen = sampleframes;
    static int display_counter = 0;
    static int display_max = 150;    // how often to display debug info
    int diff;
    
   
    t_double	*out1 = outs[0];
    t_double	*out2 = outs[1];
	
    
    //  outlets will send IQ data
	
    i = 0;
    while (sampleframes--) {
        
        if(circ_index_read > CIRCMAX) {
            circ_index_read = 0;
        }
        
        // if radio is off, just output zero's
        if(x->x_radio_is_running == 0) {
            *out1 = 0;
            *out2 = 0;
            
        }
        
        // raw mode sends baseband IQ data - everything else is demodulated audio
        // note that raw mode reads buffer twice as fast because its interleaved IQ
        else  {
            *out1 = linear_map((double) circ_buf_left[circ_index_read++], -32768, 32767, -1, 1);
            if(circ_index_read > CIRCMAX) {
                circ_index_read = 0;
            }
            *out2 = linear_map((double) circ_buf_left[circ_index_read++], -32768, 32767, -1, 1);
            
        }

        
        out1++;
        out2++;
        
        
    }
    
    
    // debugging stuff for circular buffer and latency
    
    // display_counter--;   // uncomment this line to display debug info
    if(display_counter <= 0) {
        display_counter = display_max;
        if( circ_index_write > circ_index_read) {
            diff = circ_index_write - circ_index_read;
        }
        else {
            diff = (circ_index_write + CIRCMAX) - circ_index_read;
        }
        sprintf(mesg, "wndx %d:, rndx: %d, diff: %d veclen: %d", circ_index_write, circ_index_read, diff, veclen  );
        post(mesg, 0);
        
    }
    
}

#endif


//////////////////////////////////
//
// object deconstructor routine
//

#ifdef PDVERSION
void rtlsdr_free( t_rtlsdr *x) {
    
    // here we will need to free up allocated buffer memory, once it is made local to the object
    
    // dsp_free((t_pxobject *) x);
	// freebytes();	// this will be used once we are properly allocating memory in Pd
    rtlsdr_already_loaded = 0;  // make room for another object
    
}
#else
void rtlsdr_free( t_rtlsdr *x) {
    
    // here we will need to free up allocated buffer memory, once it is made local to the object
    
    dsp_free((t_pxobject *) x);
    rtlsdr_already_loaded = 0;  // make room for another object
    
}
#endif


////////////////////////////////////
//
// Max assist routine
//
#ifndef PDVERSION
void rtlsdr_assist(t_rtlsdr *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET) {
		switch (a) {
			case 0:
				sprintf(s,"various messages");
				break;
                
		}
	}
	else
		sprintf(s,"radio - IQ signal output");
}
#endif

/////////////////////////////////
//
//  input handlers
//
// note - you can use identical handlers (max/pd) for messages in A_GIMME format like
// the one for "freq"
//


// freq message - sets the radio frequency in Hz (Max and Pd)
//
// format: freq <frequency in Hz> [now]
//
// the optional 'now' argument, if equal to 1,  sets frequency right now, if radio is running.
// if argument is not passed or equals zero (default) freq setting will be deferred until the radio
// gets reset.
//
void rtlsdr_freq (t_rtlsdr *x, t_symbol *mesg, short argc, t_atom *argv ) {
    
    // char msg[100];
    int now = 0;      // default is to defer frequency setting until reset
    
    if(argc < 1) {
        post("freq: <frequency in Hz> [now]", 0);
        return;
    }
    
    if(argc > 0) {
        x->x_freq_hz = (long) atom_getfloatarg(0, argc,argv);
        new_freq = (uint32_t) x->x_freq_hz;
		// sprintf(msg, "freq: setting freq to: %f", x->x_freq_hz);
        // post(msg, 0);
    }
    
    
    if(argc > 1 ) {
        now = (int) atom_getfloatarg(1, argc,argv);
    }
    
    if(now == 0) {
		// post("freq: deferred");
        return;         // don't set frequency now
    }
    
	// post("freq: immediate");
	
    // note, if radio is off, nothing happens until its turned on
    
    if(x->x_radio_is_running == 1) {
        need_to_set_freq = 1;
    }
    
    
}

// gain message - sets the radio gain in dB (Max and Pd)
//
// format: gain <gain in db> [now]
// (note you can also set to "auto"
//
// the optional 'now' argument, if equal to 1,  sets gain right now, if radio is running.
// if argument is not passed or equals zero (default) gain setting will be deferred until the radio
// gets reset.
//
void rtlsdr_gain (t_rtlsdr *x, t_symbol *mesg, short argc, t_atom *argv ) {
    
    // char msg[100];
    int now = 0;      // default is to defer setting until reset
    t_symbol *sym;   // special param for auto gain setting
    
    
    if(argc < 1) {
        post("freq: <gain in db> [now]", 0);
        return;
    }
    
    
    //
    if(argc > 0) {
        // first check for 'auto' gain
        // Max and Pd have different names for function to retrive symbols...
#ifdef PDVERSION
        sym = atom_getsymbolarg(0, argc,argv);
#else
        sym = atom_getsymarg(0, argc,argv);
#endif
        if(strcmp(sym->s_name,"auto") == 0) {
            x->x_gain_db10 = AUTO_GAIN;
        }
        else {  // not auto - so parse as float val
            x->x_gain_db10 = (int) (atom_getfloatarg(0, argc,argv) * 10);
           
        }
    new_gain = x->x_gain_db10;
    // sprintf(msg, "freq: setting freq to: %f", x->x_freq_hz);
    // post(msg, 0);
    }
    if(argc > 1 ) {
        now = (int) atom_getfloatarg(1, argc,argv);
    }
    
    // tz - note all gain settings deferred right now - during testing
    
    if(now == 0) {
		// post("gain: deferred");
        return;         // don't set now
    }
    
	// post("gain: immediate");
	
    // note, if radio is off, nothing happens until its turned on
    
    if(x->x_radio_is_running == 1) {
        need_to_set_gain = 1;
    }
    
    
}






//
// info message (Max and Pd)
//
// gives information about radio device, as well as max settings
//
void rtlsdr_info (t_rtlsdr *x ) {
    
    // int err;
	post("info");
    
    return;
    
}



// samplerate message - set device output sample rate (Max and Pd)
//
// this is the -s option in rtl_fm
//
// It defaults to the Max/Pd sample rate if not set otherwise
// If you set to something other than Max/Pd SR, expect weird results unless
// you use the resample option (-r) to match the Max/Pd SR
//
// the device 'input' sample rate is automatically set at the first multiple
// of samplerate that is above 1 Mhz - this is an rtl_fm thing...
//
void rtlsdr_samplerate (t_rtlsdr *x, t_symbol *mesg, short argc, t_atom *argv) {
    
   	char msg[100];
	long rate;
    
    if(argc < 1) {
        post("samplerate: <device-output-sample-rate>", 0);
        return;
    }
    
	rate = (long) atom_getfloatarg(0, argc,argv);
    x->x_device_samplerate =  rate;    // we'll use this eventually
    
    
    sprintf(msg, "setting device samplerate to: %ld", x->x_device_samplerate);
    post(msg, 0);
    
    
}




// resetbuf message (Max and Pd)
//
void rtlsdr_resetbuf (t_rtlsdr *x ) {
    
    char msg[100];
    // should do range check here
    
    // x->x_freq_hz =  freq_hz;    // we'll use this eventually
    
    // need to make sure the radio is on? or make sure that gets checked somewhere
    
    // new_freq = freq_hz;
    need_to_reset_circ_buffer = 1;
    sprintf(msg, "manually resetting circular buffer");
    post(msg, 0);
    
    
}

// start message (Max and Pd)
//
//
void rtlsdr_start (t_rtlsdr *x ) {
    
    
    char msg[100];
    int err = 1;
        
    rtlsdr_stop(x); // stop radio if its running
    
   // x->x_raw_mode = 1;    // now this will always be the case
    
    err = start_the_radio(0, x->x_freq_hz,  x->x_device_samplerate, x->x_gain_db10);
   
   
    if(err == 0) {
        x->x_radio_is_running = 1;
        sprintf(msg, "Radio started. Status: %d", err );
    }
    else {
        x->x_radio_is_running = 0;
        sprintf(msg, "Radio did not start. Status: %d", err );
    }
    
    
    post(msg, 0);
    
    //
    
}


// stop message (Max and Pd)
//
// stop the radio
//
void rtlsdr_stop (t_rtlsdr *x ) {
    
    char msg[100];
    int err;
    
    // note: even if an error occurs while stopping, we will stop the output signal
    
    if(x->x_radio_is_running) {
        x->x_radio_is_running = 0;
        err = stop_the_radio();     // non zero is an error
        sprintf( msg, "Radio is stopped. Status: %d", err);
        post(msg, 0);
        
    }
    
}




#ifndef PDVERSION	// max only
// the float and int routines cover both inlets.  It doesn't matter which one is involved
void rtlsdr_float(t_rtlsdr *x, double f)
{
    t_int option;
    
	x->x_val = f;
    
    option = (t_int)f;
    
    
    
    
    
}

//
//
// unused - just left in case we decide to
//
//
void rtlsdr_int(t_rtlsdr *x, long n)
{
    
    
	x->x_val = n;
    
    
}
#endif // Max only input handlers


/////////////////////////////////
//
// miscellaneous utilities
//

// map one range to another
float linear_map( float val, float leftMin, float leftMax , float rightMin, float rightMax) {
    
    float leftSpan, rightSpan, valueScaled;
    
    // Figure out how 'wide' each range is
    leftSpan = leftMax - leftMin;
    rightSpan = rightMax - rightMin;
    
    
    // Convert the left range into a 0-1 range (float)
    valueScaled = (val - leftMin) / leftSpan;
    
    // Convert the 0-1 range into a value in the right range.
    return rightMin + (valueScaled * rightSpan);
    
    
}
