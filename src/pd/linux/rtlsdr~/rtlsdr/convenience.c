/*
 * Copyright (C) 2014 by Kyle Keen <keenerd@gmail.com>
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

/* a collection of user friendly tools
 * todo: use strtol for more flexible int parsing
 * */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#define _USE_MATH_DEFINES
#endif

#include <math.h>

#include "rtl-sdr.h"

#define PDVERSION
#ifdef PDVERSION
#include "m_pd.h"
#else
#include "ext.h"        // tz max/msp
#include "ext_obex.h"
#include "z_dsp.h"
#endif

extern char errmesg[];  // tz error message string to replace fprintf calls


double atofs(char *s)
/* standard suffixes */
{
	char last;
	int len;
	double suff = 1.0;
	len = strlen(s);
	last = s[len-1];
	s[len-1] = '\0';
	switch (last) {
		case 'g':
		case 'G':
			suff *= 1e3;
		case 'm':
		case 'M':
			suff *= 1e3;
		case 'k':
		case 'K':
			suff *= 1e3;
			suff *= atof(s);
			s[len-1] = last;
			return suff;
	}
	s[len-1] = last;
	return atof(s);
}

double atoft(char *s)
/* time suffixes, returns seconds */
{
	char last;
	int len;
	double suff = 1.0;
	len = strlen(s);
	last = s[len-1];
	s[len-1] = '\0';
	switch (last) {
		case 'h':
		case 'H':
			suff *= 60;
		case 'm':
		case 'M':
			suff *= 60;
		case 's':
		case 'S':
			suff *= atof(s);
			s[len-1] = last;
			return suff;
	}
	s[len-1] = last;
	return atof(s);
}

double atofp(char *s)
/* percent suffixes */
{
	char last;
	int len;
	double suff = 1.0;
	len = strlen(s);
	last = s[len-1];
	s[len-1] = '\0';
	switch (last) {
		case '%':
			suff *= 0.01;
			suff *= atof(s);
			s[len-1] = last;
			return suff;
	}
	s[len-1] = last;
	return atof(s);
}

int nearest_gain(rtlsdr_dev_t *dev, int target_gain)
{
	int i, r, err1, err2, count, nearest;
	int* gains;
	r = rtlsdr_set_tuner_gain_mode(dev, 1);
	if (r < 0) {
		sprintf(errmesg, "WARNING: Failed to enable manual gain.\n");
        post(errmesg,0);
		return r;
	}
	count = rtlsdr_get_tuner_gains(dev, NULL);
	if (count <= 0) {
		return 0;
	}
	gains = malloc(sizeof(int) * count);
	count = rtlsdr_get_tuner_gains(dev, gains);
	nearest = gains[0];
	for (i=0; i<count; i++) {
		err1 = abs(target_gain - nearest);
		err2 = abs(target_gain - gains[i]);
		if (err2 < err1) {
			nearest = gains[i];
		}
	}
	free(gains);
	return nearest;
}

int verbose_set_frequency(rtlsdr_dev_t *dev, uint32_t frequency)
{
	int r;
	r = rtlsdr_set_center_freq(dev, frequency);
	if (r < 0) {
		sprintf(errmesg, "WARNING: Failed to set center freq.\n");
	} else {
		sprintf(errmesg, "Tuned to %u Hz.\n", frequency);
	}
    post(errmesg,0);
	return r;
}

int verbose_set_sample_rate(rtlsdr_dev_t *dev, uint32_t samp_rate)
{
	int r;
	r = rtlsdr_set_sample_rate(dev, samp_rate);
	if (r < 0) {
		sprintf(errmesg, "WARNING: Failed to set sample rate.\n");
	} else {
		sprintf(errmesg, "Sampling at %u S/s.\n", samp_rate);
	}
    post(errmesg,0);
	return r;
}

int verbose_direct_sampling(rtlsdr_dev_t *dev, int on)
{
	int r;
	r = rtlsdr_set_direct_sampling(dev, on);
	if (r != 0) {
		sprintf(errmesg, "WARNING: Failed to set direct sampling mode.\n");
        post(errmesg,0);
		return r;
	}
	if (on == 0) {
		sprintf(errmesg, "Direct sampling mode disabled.\n");}
	if (on == 1) {
		sprintf(errmesg, "Enabled direct sampling mode, input 1/I.\n");}
	if (on == 2) {
		sprintf(errmesg, "Enabled direct sampling mode, input 2/Q.\n");}
    post(errmesg,0);
	return r;
}

int verbose_offset_tuning(rtlsdr_dev_t *dev)
{
	int r;
	r = rtlsdr_set_offset_tuning(dev, 1);
	if (r != 0) {
		sprintf(errmesg, "WARNING: Failed to set offset tuning.\n");
	} else {
		sprintf(errmesg, "Offset tuning mode enabled.\n");
	}
    post(errmesg,0);
	return r;
}

int verbose_auto_gain(rtlsdr_dev_t *dev)
{
	int r;
	r = rtlsdr_set_tuner_gain_mode(dev, 0);
	if (r != 0) {
		sprintf(errmesg, "WARNING: Failed to set tuner gain.\n");
	} else {
		sprintf(errmesg, "Tuner gain set to automatic.\n");
	}
    post(errmesg,0);
	return r;
}

int verbose_gain_set(rtlsdr_dev_t *dev, int gain)
{
	int r;
	r = rtlsdr_set_tuner_gain_mode(dev, 1);
	if (r < 0) {
		sprintf(errmesg, "WARNING: Failed to enable manual gain.\n");
        post(errmesg,0);
		return r;
	}
	r = rtlsdr_set_tuner_gain(dev, gain);
	if (r != 0) {
		sprintf(errmesg, "WARNING: Failed to set tuner gain.\n");
	} else {
		sprintf(errmesg, "Tuner gain set to %0.2f dB.\n", gain/10.0);
	}
    post(errmesg,0);
	return r;
}

int verbose_ppm_set(rtlsdr_dev_t *dev, int ppm_error)
{
	int r;
	if (ppm_error == 0) {
		return 0;}
	r = rtlsdr_set_freq_correction(dev, ppm_error);
	if (r < 0) {
		sprintf(errmesg, "WARNING: Failed to set ppm error.\n");
	} else {
		sprintf(errmesg, "Tuner error set to %i ppm.\n", ppm_error);
	}
    post(errmesg,0);
	return r;
}

int verbose_reset_buffer(rtlsdr_dev_t *dev)
{
	int r;
	r = rtlsdr_reset_buffer(dev);
	if (r < 0) {
		sprintf(errmesg, "WARNING: Failed to reset buffers.\n");}
    post(errmesg,0);
	return r;
}

int verbose_device_search(char *s)
{
	int i, device_count, device, offset;
	char *s2;
	char vendor[256], product[256], serial[256];
	device_count = rtlsdr_get_device_count();
	if (!device_count) {
		sprintf(errmesg, "No supported devices found.\n");
        post(errmesg,0);
		return -1;
	}
	sprintf(errmesg, "Found %d device(s):\n", device_count);
	for (i = 0; i < device_count; i++) {
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
		sprintf(errmesg, "  %d:  %s, %s, SN: %s\n", i, vendor, product, serial);
        post(errmesg,0);
	}
	sprintf(errmesg, "\n");
    post(errmesg,0);
	/* does string look like raw id number */
	device = (int)strtol(s, &s2, 0);
	if (s2[0] == '\0' && device >= 0 && device < device_count) {
		sprintf(errmesg, "Using device %d: %s\n",
			device, rtlsdr_get_device_name((uint32_t)device));
        post(errmesg,0);
		return device;
	}
	/* does string exact match a serial */
	for (i = 0; i < device_count; i++) {
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
		if (strcmp(s, serial) != 0) {
			continue;}
		device = i;
		sprintf(errmesg, "Using device %d: %s\n",
			device, rtlsdr_get_device_name((uint32_t)device));
        post(errmesg,0);
		return device;
	}
	/* does string prefix match a serial */
	for (i = 0; i < device_count; i++) {
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
		if (strncmp(s, serial, strlen(s)) != 0) {
			continue;}
		device = i;
		sprintf(errmesg, "Using device %d: %s\n",
			device, rtlsdr_get_device_name((uint32_t)device));
        post(errmesg,0);
		return device;
	}
	/* does string suffix match a serial */
	for (i = 0; i < device_count; i++) {
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
		offset = strlen(serial) - strlen(s);
		if (offset < 0) {
			continue;}
		if (strncmp(s, serial+offset, strlen(s)) != 0) {
			continue;}
		device = i;
		sprintf(errmesg, "Using device %d: %s\n",
			device, rtlsdr_get_device_name((uint32_t)device));
        post(errmesg,0);
		return device;
	}
	sprintf(errmesg, "No matching devices found.\n");
    post(errmesg,0);
	return -1;
}

// vim: tabstop=8:softtabstop=8:shiftwidth=8:noexpandtab
