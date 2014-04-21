// tz - this file is based on rtl_fm.c from feb 26/2014 from the rtlsdr archive
// tz this is the Max version from 3/28/2014 brought over to Pd to adapt it

/*
 * rtl-sdr, turns your Realtek RTL2832 based DVB dongle into a SDR receiver
 * Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
 * Copyright (C) 2012 by Hoernchen <la@tfc-server.de>
 * Copyright (C) 2012 by Kyle Keen <keenerd@gmail.com>
 * Copyright (C) 2013 by Elias Oenal <EliasOenal@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// tz notes: 4/17/2014
// now have simplified the Max/Pd interface to work with raw samples
//
// but have not yet dismantled all the structs for demodulation and scanning
//

#define PDVERSION    // uncomment this for Pd

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include "getopt/getopt.h"
#define usleep(x) Sleep(x/1000)
#ifdef _MSC_VER
#define round(x) (x > 0.0 ? floor(x + 0.5): ceil(x - 0.5))
#endif
#define _USE_MATH_DEFINES
#endif

#include <math.h>
#include <pthread.h>
#include <libusb.h>

#include "rtl-sdr.h"

#ifdef PDVERSION
#include "m_pd.h"
#else
#include "ext.h"        // tz max/msp
#include "ext_obex.h"
#include "z_dsp.h"
#endif

#include "convenience.h"        // tz I made this .h local

// standard Max external stuff


char errmesg[2000];  // tz error message string to replace fprintf calls
int radio_running = 0;  // radio state indicator

volatile uint32_t new_freq;
volatile int need_to_set_freq = 0;

volatile int new_gain;
volatile int need_to_set_gain = 0;

char new_mode[10] = "fm";
volatile int need_to_set_mode = 0;

volatile int need_to_reset_circ_buffer = 0;
// volatile int need_to_restart = 0;

// more prototypes

void reset_demod(char *demodtype);
void buffer_init(void);
int start_the_radio(int device_index, long freq_hz, long output_SR, int gain_db10);
int stop_the_radio(void);

#define CIRCMAX 1500000

int16_t circ_buf_left[CIRCMAX * sizeof(int16_t) + 1];

int circ_index_write = 0;
int circ_index_read = 0;
int circ_buf_latency_factor = 2; // number of vectors-1



#define DEFAULT_SAMPLE_RATE		24000
#define DEFAULT_BUF_LENGTH		(1 * 16384)
#define MAXIMUM_OVERSAMPLE		16
#define MAXIMUM_BUF_LENGTH		(MAXIMUM_OVERSAMPLE * DEFAULT_BUF_LENGTH)
#define AUTO_GAIN			-100
#define BUFFER_DUMP			4096

#define FREQUENCIES_LIMIT		1000

static volatile int do_exit = 0;

static int ACTUAL_BUF_LENGTH;


struct dongle_state
{
	int      exit_flag;
	pthread_t thread;
	rtlsdr_dev_t *dev;
	int      dev_index;
	uint32_t freq;
	uint32_t rate;
	int      gain;
	uint16_t buf16[MAXIMUM_BUF_LENGTH];
	uint32_t buf_len;
	int      ppm_error;
	int      offset_tuning;
	int      direct_sampling;
	int      mute;
	struct demod_state *demod_target;
};

struct demod_state
{
	int      exit_flag;
	pthread_t thread;
	int16_t  lowpassed[MAXIMUM_BUF_LENGTH];
	int      lp_len;
	int16_t  lp_i_hist[10][6];
	int16_t  lp_q_hist[10][6];
	int16_t  result[MAXIMUM_BUF_LENGTH];
	int16_t  droop_i_hist[9];
	int16_t  droop_q_hist[9];
	int      result_len;
	int      rate_in;
	int      rate_out;
	int      rate_out2;
	int      now_r, now_j;
	int      pre_r, pre_j;
	int      prev_index;
	int      downsample;    /* min 1, max 256 */
	int      post_downsample;
	int      output_scale;
	int      squelch_level, conseq_squelch, squelch_hits, terminate_on_squelch;
	int      downsample_passes;
	int      comp_fir_size;
	int      custom_atan;
	int      deemph, deemph_a;
	int      now_lpr;
	int      prev_lpr_index;
	int      dc_block, dc_avg;
	void     (*mode_demod)(struct demod_state*);
	pthread_rwlock_t rw;
	pthread_cond_t ready;
	pthread_mutex_t ready_m;
	struct output_state *output_target;
};

// tz this would be a good place for the circular audio buffers
// because the structure gets passed to the thread where the output file is written
//
struct output_state
{
	int      exit_flag;
	pthread_t thread;
	FILE     *file;
	char     *filename;
	int16_t  result[MAXIMUM_BUF_LENGTH];
	int      result_len;
	int      rate;
	pthread_rwlock_t rw;
	pthread_cond_t ready;
	pthread_mutex_t ready_m;
};

// tz this is where we will change the frequency
//
//

struct controller_state
{
	int      exit_flag;
	pthread_t thread;
	uint32_t freqs[FREQUENCIES_LIMIT];  // list of frequencies (for scanning)
	int      freq_len;      // tz number of frequencies in the list
	int      freq_now;
	int      edge;
	int      wb_mode;
	pthread_cond_t hop;
	pthread_mutex_t hop_m;
    // tz new fields for external frequency control
    uint32_t new_freq;
    int     need_to_set_freq;
    
};

// multiple of these, eventually
struct dongle_state dongle;
struct demod_state demod;
struct output_state output;
struct controller_state controller;

// tz more function prototypes
void set_tuner_gain(struct dongle_state *d );

void copy_samples_to_circ_buffer( uint16_t *source, int len, int downsample );


/* more cond dumbness */
#define safe_cond_signal(n, m) pthread_mutex_lock(m); pthread_cond_signal(n); pthread_mutex_unlock(m)
#define safe_cond_wait(n, m) pthread_mutex_lock(m); pthread_cond_wait(n, m); pthread_mutex_unlock(m)



#ifdef _MSC_VER
double log2(double n)
{
	return log(n) / log(2.0);
}
#endif

// tz - what exactly is this doing? It gets called if radio is not in 'offset' mode
//
void rotate_90(unsigned char *buf, uint32_t len)
/* 90 rotation is 1+0j, 0+1j, -1+0j, 0-1j
 or [0, 1, -3, 2, -4, -5, 7, -6] */
{
	uint32_t i;
	unsigned char tmp;
	for (i=0; i<len; i+=8) {
		/* uint8_t negation = 255 - x */
		tmp = 255 - buf[i+3];
		buf[i+3] = buf[i+2];
		buf[i+2] = tmp;
        
		buf[i+4] = 255 - buf[i+4];
		buf[i+5] = 255 - buf[i+5];
        
		tmp = 255 - buf[i+6];
		buf[i+6] = buf[i+7];
		buf[i+7] = tmp;
	}
}



//  copies raw samples into Max/Pd circ buffer at current downsampling factor
//
//
// simple square window FIR
void copy_samples_to_circ_buffer( uint16_t *source, int len, int downsample ) {
    
    int i=0, i2=0;
    
    // simply using this struct to keep track of static variables between callbacks
    struct demod_state *d = &demod;
    
	while (i < len) {
        
        d->now_r += source[i];
		d->now_j += source[i+1];
		i += 2;
		d->prev_index++;
		if (d->prev_index < d->downsample) {
			continue;
		}
		// d->lowpassed[i2]   = d->now_r; // * d->output_scale;
		// d->lowpassed[i2+1] = d->now_j; // * d->output_scale;
        
        if(circ_index_write > CIRCMAX) {
            circ_index_write = 0;
        }
        circ_buf_left[circ_index_write++] = d->now_r;
        if(circ_index_write > CIRCMAX) {
            circ_index_write = 0;
        }
        circ_buf_left[circ_index_write++] = d->now_j;
		
        
        d->prev_index = 0;
		d->now_r = 0;
		d->now_j = 0;
		i2 += 2;
	}
	d->lp_len = i2;
    
}

// device callback: copies chunk of samples to buffer
static void rtlsdr_callback(unsigned char *buf, uint32_t len, void *ctx)
{
	int i;
	struct dongle_state *s = ctx;
	struct demod_state *d = s->demod_target;
    
	if (do_exit) {
		return;}
	if (!ctx) {
		return;}
	if (s->mute) {
		for (i=0; i<s->mute; i++) {
			buf[i] = 127;}
		s->mute = 0;
	}
	if (!s->offset_tuning) {
		rotate_90(buf, len);}
    
    // tz apparently you need to subtract 127 from each sample to do some kind of
    // "normalization" of the amplitude range - like a DC offset adjustment?
    
	for (i=0; i<(int)len; i++) {
		s->buf16[i] = (int16_t)buf[i] - 127;}
	
    pthread_rwlock_wrlock(&d->rw);
    // tz - it seems like here, we could copy the raw samples to Max/Pd?
    // but we would need to know the sample rate.

    // zzz
	//memcpy(d->lowpassed, s->buf16, 2*len); // is the size of each sample 2 bytes?
	//d->lp_len = len;

// can we get upsample factor here?
// ok this is succesfull - we have eliminated the demod and output threads, but if
// there are problems - this call should probably be put back on another thread
// all its doing is summing the samples and writing to a buffer though
    
    copy_samples_to_circ_buffer( s->buf16, len, 1 );
    
    pthread_rwlock_unlock(&d->rw);
	safe_cond_signal(&d->ready, &d->ready_m);   // tz do we still need this?
    safe_cond_signal(&controller.hop, &controller.hop_m); // allows controller changes
}

// tz this is the thread where actual device reads take place
// well actually they happen in the callback
//
static void *dongle_thread_fn(void *arg)
{
	struct dongle_state *s = arg;
	rtlsdr_read_async(s->dev, rtlsdr_callback, s, 0, s->buf_len);
	return 0;
}



// tz - call this function everytime the frequency changes
//

static void optimal_settings_new(int freq, int rate)
{
	// giant ball of hacks
	
	int capture_freq, capture_rate;
	struct dongle_state *d = &dongle;
	struct demod_state *dm = &demod;
	// struct controller_state *cs = &controller;
    
        // downsample is the mysterious factor that explains everything
    // first of all, its an int value between 1->256
    //
    // so, for specified SR's (in_rate) less than a million, the device actually runs at
    // around a million and then does a simple decimation to the actual (in_rate) which is
    // actually the output rate, for all practical purposes - this downsampling happens in
    // low_pass() which happens inside full_demod()
    //
    // typical downsample values:
    //
    // rate_in = 96k : 11
    // rate_in = 170k : 6
    //
	dm->downsample = (1000000 / dm->rate_in) + 1;

    
    // tz typical capture rate values:
    //
    // rate_in = 96k : 1,056,000
    // rate_in = 170k : 1,020,000
    //
    //
	capture_freq = freq;
	capture_rate = dm->downsample * dm->rate_in;
    //
    // tz - lets skip offset tuning for now... it is only used if explicitly
    // enable with -E offset
    //
    // so: capture_freq is slightly different than actual freq... not sure why - but lets say you
    // are sampling at 1mhz, then capture_rate / 4 = 250k and so you add that to freq to get
    // capture freq which at 94900000 would be 95,150,000 - ahhhh this explains it
    //
	// if (!d->offset_tuning) {
	
	capture_freq = freq + capture_rate/4;
    
    //}
    //
    // tz - edge is an undocumented feature enabled by -E edge
    // we will assume it is 0 for now
    // but it looks like it pushes the capture freq away from the edge of bandwidth
    //
	//capture_freq += cs->edge * dm->rate_in / 2;
    //
    
    
    //
    // tz - so after all this, the dongle freq is higher than original freq, depending on
    // amount of downsampling
    // and the dongle rate is probably, slightly over a million
    //
	d->freq = (uint32_t)capture_freq;
	d->rate = (uint32_t)capture_rate;
}











// tz this is where gain and frequency are set
//
//
static void *controller_thread_fn(void *arg)
{
	// from original comments:
    // thoughts for multiple dongles
	// might be no good using a controller thread if retune/rate blocks
	
    // int i;
	struct controller_state *s = arg;
    static int latency_timer = 0;
    

    // tz - note that in optimal_settings the actual frequency in s-> is used to set the dongle.freq
    // then the dongle.freq what gets sent to the device.
    //
    //
	optimal_settings_new(s->freqs[0], demod.rate_in);
    
    // tz - we have temporarily abandoned these options but here is where you would set them
    //
	// if (dongle.direct_sampling) {
	//  	verbose_direct_sampling(dongle.dev, 1);}
	// if (dongle.offset_tuning) {
	//  	verbose_offset_tuning(dongle.dev);}
    
	/* Set the frequency */
	
    verbose_set_frequency(dongle.dev, dongle.freq);

	sprintf(errmesg, "Oversampling input by: %ix.\n", demod.downsample);
    post(errmesg,0);
	sprintf(errmesg, "Oversampling output by: %ix.\n", demod.post_downsample);
    post(errmesg,0);
	sprintf(errmesg, "Buffer size: %0.2fms\n",
            1000 * 0.5 * (float)ACTUAL_BUF_LENGTH / (float)dongle.rate);
    post(errmesg,0);
    
	/* Set the sample rate */
	verbose_set_sample_rate(dongle.dev, dongle.rate);
	sprintf(errmesg, "Output at %u Hz.\n", demod.rate_in/demod.post_downsample);
    post(errmesg,0);
    
    
    need_to_reset_circ_buffer = 1;  // tz - this will cause read buffer to reset
    
    
	while (!do_exit) {
        //usleep(100000);
        //post("in controller thread", 0);
        
		safe_cond_wait(&s->hop, &s->hop_m);
        // tz it might be possible to set 'mode' here - using the same init
		// sequence as with restarting the radio
        
        if(need_to_set_freq) {
            need_to_set_freq = 0;
            
            optimal_settings_new(new_freq, demod.rate_in);
            rtlsdr_set_center_freq(dongle.dev, dongle.freq);
            //sprintf(errmesg, "setting frequency to %d", new_freq);
            //post(errmesg, 0);
            
            // need_to_reset_circ_buffer = 1; // not sure if we need to do this here too...
            
            // post("hello...");
            
        }
        
        if(need_to_set_gain) {
            need_to_set_gain = 0;
            dongle.gain = new_gain;
            sprintf(errmesg, "setting gain to %d", new_gain);
            post(errmesg, 0);
            set_tuner_gain(&dongle);
            
        }
        
        if(need_to_reset_circ_buffer) {
            need_to_reset_circ_buffer = 0;
            latency_timer = circ_buf_latency_factor + 1;
            circ_index_write = 0;
            post("reseting circular buffer write index", 0);
        }
        
        // After latency timer counts down to 1, reset the read index too
        // timer cound represents number of vectors (determined by latency factor)
        if(latency_timer > 0) {
            if(latency_timer == 1) {
                post("reseting circular buffer read index", 0);
                circ_index_read = 0;
            }
            latency_timer--;
        }

     
        
  	}
	return 0;
   
}



void dongle_init(struct dongle_state *s)
{
	s->rate = DEFAULT_SAMPLE_RATE;
	s->gain = AUTO_GAIN; // tenths of a dB
	s->mute = 0;
	s->direct_sampling = 0;
	s->offset_tuning = 0;
	s->demod_target = &demod;
}

void demod_init(struct demod_state *s)
{
	s->rate_in = DEFAULT_SAMPLE_RATE;
	s->rate_out = DEFAULT_SAMPLE_RATE;
	s->squelch_level = 0;
	s->conseq_squelch = 10;
	s->terminate_on_squelch = 0;
	s->squelch_hits = 11;
	s->downsample_passes = 0;
	s->comp_fir_size = 0;
	s->prev_index = 0;
	s->post_downsample = 1;  // once this works, default = 4
	s->custom_atan = 0;
	s->deemph = 0;
	s->rate_out2 = -1;  // flag for disabled
	s->mode_demod = NULL;
	s->pre_j = s->pre_r = s->now_r = s->now_j = 0;
	s->prev_lpr_index = 0;
	s->deemph_a = 0;
	s->now_lpr = 0;
	s->dc_block = 0;
	s->dc_avg = 0;
//	pthread_rwlock_init(&s->rw, NULL);
//	pthread_cond_init(&s->ready, NULL);
//	pthread_mutex_init(&s->ready_m, NULL);
	s->output_target = &output;
}



void output_init(struct output_state *s)
{
	s->rate = DEFAULT_SAMPLE_RATE;
//	pthread_rwlock_init(&s->rw, NULL);
//	pthread_cond_init(&s->ready, NULL);
//	pthread_mutex_init(&s->ready_m, NULL);
}

// tz some cleanup
void buffer_init() {
    circ_index_write = 0;
    circ_index_read = 0;
}


void controller_init(struct controller_state *s)
{
	s->freqs[0] = 100000000;
	s->freq_len = 0;
	s->edge = 0;
	s->wb_mode = 0;
	pthread_cond_init(&s->hop, NULL);
	pthread_mutex_init(&s->hop_m, NULL);
}

void controller_cleanup(struct controller_state *s)
{
	pthread_cond_destroy(&s->hop);
	pthread_mutex_destroy(&s->hop_m);
}



///////////////////////
//
//
int start_the_radio(int device_index, long freq_hz, long output_SR, int gain_db10) {
    

        
    
//         post("inside start_the_radio()" );
    
        
       // int err = 0;    // tz for usage call
        
    
    int r;
    
    int dev_given = 0;  // gets set on, if user passes in device index arg (-d)
    // int custom_ppm = 0; // gets set on, if users specifies freq correction arg (-p)
    char device_index_str[10] = "0";
        
// initialization
    
        dongle_init(&dongle);
        demod_init(&demod);
        output_init(&output);
        controller_init(&controller);
        buffer_init();          // tz reset index pointers
        
// handle device index
    
    if(device_index != 0) {
        sprintf(device_index_str,"%d", device_index);
        dongle.dev_index = verbose_device_search(device_index_str);
        dev_given = 1;
    }
    
// set gain
    
  
    // tz - note that when passing this param from Max/Pd it gets passed in
    // db * 10 format - so we don't need extra multiplier
    // this is done to allow setting to AUTOGAIN
    //
    dongle.gain = gain_db10 ;
    
    
// set sample rate
    
    demod.rate_in = (uint32_t) output_SR;
    demod.rate_out = (uint32_t) output_SR;

// set frequency
    
    controller.freqs[controller.freq_len] = (uint32_t) freq_hz;
    controller.freq_len++;
    
    
// note: here is how dongle ppm error correction was set in rtl_fm
    
    // dongle.ppm_error = atoi(optarg);
    // custom_ppm = 1;
    
// do initial setup
        

// tz this was for resampling and can be removed
    
        output.rate = demod.rate_out;
    
    
// tz this can get cleaned up too...
    
        // set buffer length - would need to increase if downsample rate increased
        //
        ACTUAL_BUF_LENGTH = DEFAULT_BUF_LENGTH;
        
        // if user specied a device number, use it - otherwise default to 0
        if (!dev_given) {
            dongle.dev_index = verbose_device_search("0");
        }
        
        if (dongle.dev_index < 0) {
            return(1);
        }
        
        // open device
        r = rtlsdr_open(&dongle.dev, (uint32_t)dongle.dev_index);
        if (r < 0) {
            // sprintf(errmesg, "Failed to open rtlsdr device #%d.\n", dongle.dev_index);
            sprintf(errmesg, "Failed to open rtlsdr device #%d.\n", dongle.dev_index);
            post(errmesg, 0);
            return(1);
        }
        
   
        
        set_tuner_gain(&dongle);
        
        
        // do freq error correction
        verbose_ppm_set(dongle.dev, dongle.ppm_error);
        
    
        //r = rtlsdr_set_testmode(dongle.dev, 1);
        
        /* Reset endpoint before we start reading from it (mandatory) */
        verbose_reset_buffer(dongle.dev);
        
        // tz these threads all get running in parallel here
        //
        int errx;   // for debugging - not really used now
        
        errx = pthread_create(&controller.thread, NULL, controller_thread_fn, (void *)(&controller));
        usleep(100000);

        errx = pthread_create(&dongle.thread, NULL, dongle_thread_fn, (void *)(&dongle));
        
    
        radio_running = 1;
        
        return(0);

    
}




// cancel all read operations
//
int stop_the_radio()
{
    
    int i;
    
    // need to make sure the radio is actually running
    
    
    if(!radio_running) {
        return(0);
    }
    else {
        radio_running = 0;
        do_exit = 1;
    }
    
    post("stopping...", 0);
    
	if (do_exit) {
		sprintf(errmesg, "\nUser cancel, exiting...\n");
        post(errmesg,0);
    }
    
    // we'll figure out where library error goes later - apparently when the library
    // error occurs, control breaks out of the read loop but do_exit is not set tz
    /*
     else {
     sprintf(errmesg, "\nLibrary error %d, exiting...\n", r);
     post(errmesg,0);
     }
     */
    
    
	rtlsdr_cancel_async(dongle.dev);
    post("merging threads...");
	pthread_join(dongle.thread, NULL);

    safe_cond_signal(&controller.hop, &controller.hop_m);
	pthread_join(controller.thread, NULL);
    
    // cleanup thread overhead
    
    post("cleanup...");
	//dongle_cleanup(&dongle);

	controller_cleanup(&controller);
 
	rtlsdr_close(dongle.dev);
	// return r >= 0 ? r : -r;
    
    do_exit = 0;    // reset thread kill flag
    
    // zero out the circular buffer; - should probably use memset
    
    post("clearing buffers...");
    for(i = 0 ; i < CIRCMAX; i++ ) {
        circ_buf_left[i] = 0;
    }
    
    return(0);
    
    
    
}

// tz gain function
//
// should probably have option for verbose mode and possibly return actual gain
//
void set_tuner_gain(struct dongle_state *d )
{
	/* Set the tuner gain */
	if (d->gain == AUTO_GAIN) {
		verbose_auto_gain(d->dev);
	} else {
		d->gain = nearest_gain(d->dev, d->gain);
		verbose_gain_set(d->dev, d->gain);
	}
    
}

// vim: tabstop=8:softtabstop=8:shiftwidth=8:noexpandtab
