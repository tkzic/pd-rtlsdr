NAME=rtlsdr~
CSYM=rtlsdr_tilde
#
#
# 5/6/2014
#
# This directory that contains this makefile needs to be a subdirectory of 
# /usr/lib/pd-extended/extra  - which is created by installing pd-extended
#
# Instructions for instaling pd-extended here: http://puredata.info/docs/faq/debian  
#
# Dependencies:
#
# libusb-1.0
# rtlsdr
#
# to install librtlsdr see: http://sdr.osmocom.org/trac/wiki/rtl-sdr
# Follow the instructions to clone the archive, run cmake, and make to build and install
#
# for libusb try sudo apt-get install libusb-1.0
#
#

USBLIBINC = -I/usr/include/libusb-1.0

SDRINC = -I./rtlsdr/include -I./
SDRSRC = ./rtlsdr/*.c
SDROBJ = rtl_fm3.o convenience.o
LIBTHREAD = -lpthread

# ----------------------- LINUX i386 -----------------------

pd_linux: $(NAME).pd_linux

.SUFFIXES: .pd_linux

LINUXCFLAGS = -DPD -O2 -funroll-loops -fomit-frame-pointer -fPIC \
    -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-unused-parameter -Wno-parentheses -Wno-switch $(CFLAGS)

LINUXINCLUDE =  -I../../src

.c.pd_linux:
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) $(SDRLIBINC) $(SDRINC) $(USBLIBINC) -c $*.c $(SDRSRC) \
	$(pkg-config --cflags libusb-1.0) $(pkg-config --cflags librtlsdr)
	$(CC) -shared -o $*.pd_linux $*.o -lc -lm $(LIBTHREAD) $(SDROBJ) \
	`pkg-config --libs libusb-1.0` `pkg-config --libs librtlsdr`
	 

# ----------------------------------------------------------

clean:
	rm -f *.o *.pd_*
