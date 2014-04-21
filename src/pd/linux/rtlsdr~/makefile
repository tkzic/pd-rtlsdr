NAME=rtlsdr~
CSYM=rtlsdr_tilde
#
#
# 4/20/2014
#
# have made local copies of the libraries for libusb and librtlsdr
# this may not work on your system
#
# Would suggest installing rtl-sdr and libusb-1.0 to get the libraries for your system
#
# to install librtlsdr see: http://sdr.osmocom.org/trac/wiki/rtl-sdr
# Follow the instructions to clone the archive, run cmake, and make to build and install
#
# for libusb try sudo apt-get install libusb-1.0
#
# Then try changing the following. These are the original locations of those libraries on Ubuntu 14.04
# You can find them on your system by using something like: sudo find / -name libusb-1.0*
#
# original locations:
#
# SDRLIBINC = -I/usr/include/libusb-1.0 
# SDRLIBS = /usr/local/lib/librtlsdr.so /usr/lib/x86_64-linux-gnu/libusb-1.0.so

#

SDRLIBINC = # -I/usr/include/libusb-1.0 -I../include
SDRLIBS = ./rtlsdr/lib/librtlsdr.so ./rtlsdr/lib/libusb-1.0.so

SDRINC = -I./rtlsdr/include
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
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) $(SDRLIBINC) $(SDRINC) -c $*.c $(SDRSRC)
	$(CC) -shared -o $*.pd_linux $*.o -lc -lm $(LIBTHREAD) $(SDROBJ) $(SDRLIBS) 
	strip --strip-unneeded $*.pd_linux
	rm -f $*.o


# ----------------------------------------------------------

clean:
	rm -f *.o *.pd_* so_locations
