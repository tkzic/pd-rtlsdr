pd-rtlsdr
=========

Pd and Max externals for rtl-SDR

April 20, 2014

This is a pre-release version. Documentation is in progress. Here are couple of demo videos:

http://youtu.be/MafEBdgM3E4

http://youtu.be/KdXjGOqu_9Q

Note: You can only run one radio and one instance of the [rtlsdr~] object. This will be fixed in future releases. 


Quick-start:
====
the [rtlsdr~] object sends out raw IQ data from an rtlSDR device.

[rtlsdr~] responds to following messages:

start : start the radio

stop : stop the radio

freq [freq in Hz] [immediate (optional)] : set frequency in Hz. If immediate is set to 1 then frequency is changed immediately. Otherwise will take effect after start message.

gain [gain in dB] [immediate (optional)] : set RF gain. If immediate is set to 1 then gain is changed immediately. Otherwise will take effect after start message. Default is AUTO. Note: -10 will also set auto.

samplerate [samplerate] : set sample rate. Takes effect after start message. default is current Pd/Max sample rate.
	
Note that the example patches run inside block~ (pd) and poly~ (max) to allow higher sample rate for better wide band FM detection.

Pd MacOS:
====
Plug in your rtlSDR radio.

In folder pd/macOS/, run rtlSDR-block.pd, with pd-extended.
 
Click on the [pd radioblock] object to open the subpatch. 

Then click the "start" message

Pd Linux:
====
Plug in your rtlSDR radio.

In folder pd/linux/, run rtlSDR-block.pd, with pd-extended. 

Click on the [pd radioblock] object to open the subpatch. 

Then click the "start" message

Note: you may need to deal with these Usb capture and root permission issues. http://zerokidz.com/ideas/?p=10462

Max MacOS:
====

Plug in your rtlSDR radio.

In folder max/, run rtlsdr-fm1.maxpat 

Click on the green toggle in the upper left corner to start DSP

You will control the radio from inside the rtlsdr-fm1-poly~.maxpat subpatch (which opens automatically)
 
click the green "start" message


Compiling from source:
====

Pd MacOS
====
copy the src/pd/macOS/rtlsdr~ folder into Contents/Resources/extra/ inside the pd-extended application bundle

Then run the makefile inside rtlsdr~ folder: sudo make

Pd MacOS
====
copy the src/pd/linux/rtlsdr~ folder into /usr/lib/pd-extended/extra/ 

Then run the makefile inside rtlsdr~ folder: sudo make

Max MacOS
====
copy the src/max/macOS/rtlsdr~ folder into MaxSDK-6.1.4/examples/audio/  

Then run the xcode project

Acknowledgement:
====
I used vast amounts of code from rtl_fm by Kyle Keen, http://kmkeen.com/rtl-demod-guide/:

rtl_fm is part of the rtl-sdr distribution.

Many thanks to:
Joseph Deken, Katja Vetter, and Fred Jan Kraan

New Blankets http://newblankets.org

Contact
====
Questions? Interested in helping out?

tkzic@megalink.net (Tom Zicarelli)
http://tomzicarelli.com

