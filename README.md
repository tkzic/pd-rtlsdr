pd-rtlsdr
=========

Pd and Max externals for rtl-SDR

May 6, 2014

This is a pre-release version. Documentation is in progress. Here are couple of demo videos:

http://youtu.be/MafEBdgM3E4

http://youtu.be/KdXjGOqu_9Q

Running Max version 6.1.6 and Pd-extended version 0.43.4

Note: You can only run one radio and one instance of the [rtlsdr~] object. This will be fixed in future releases. 

For more information about rtl-sdr: http://sdr.osmocom.org/trac/wiki/rtl-sdr


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
Note: The external was compiled for Ubuntu 14.04 - You may need to rebuild it for your system. See below.

Plug in your rtlSDR radio.

In folder pd/linux/, run rtlSDR-block.pd, with pd-extended. 

Click on the [pd radioblock] object to open the subpatch. 

Then click the "start" message

Note: you may need to deal with these Usb capture and root permission issues. http://zerokidz.com/ideas/?p=10462

Note: I was only able to get Pd to run with Alsa audio drivers at 44.1KHz - and the audio quality was generally not as nice as on MacOS. If you are able to run Pd with other drivers or sample rates I would be interested in knowing about it.

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

Pd linux (Ubuntu)
====
1. Install pd-extended and dependencies


 Instructions for instaling pd-extended here: http://puredata.info/docs/faq/debian  

 Dependencies:

 libusb-1.0
 rtlsdr

 to install librtlsdr see: http://sdr.osmocom.org/trac/wiki/rtl-sdr
 Follow the instructions to clone the archive, run cmake, and make to build and install

 for libusb try sudo apt-get install libusb-1.0



2. Make sure that an rtlsdr radio runs from the command line using rtl_fm or rtl_test:

Also please note that you may experience issues with root permission, or USB capture. See this post for a solution: http://zerokidz.com/ideas/?p=10462

3. Copy the rtlsdr~ folder to /usr/lib/pd-extended/extra (you will need root permission)

 sudo cp -r rtlsdr~ /usr/lib/pd-extended/extra

4. inside the rtlsdr~ folder run the makefile

 make

5. Then you should be able to run the sample pd patches (see above) - although you may want to remove the rtlsdr~.pd_linux external from the local directory where the pd patches are located.

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

