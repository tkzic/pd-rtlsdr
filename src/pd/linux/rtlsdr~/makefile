NAME=rtlsdr~
CSYM=rtlsdr_tilde
#
# this will be a linux prototype makefile for Pd externals - tz
#
# this is working 4/20/2014
# but needed to use shared libraries for librtlsdr and libusb-1.0
# I wonder if there is a way it will work with static? or if it makes a difference?
# 
#
# PDINCLUDE = # -I../../src

# rtlsdr stuff

# SDRLIBINC = # -I/opt/local/include/libusb-1.0 -I../include
SDRLIBINC = -I/usr/include/libusb-1.0 -I../include
# SDRLIBS = # /usr/local/lib/libusb32-1.0.a ./rtlsdr/lib/librtlsdr32.a
# SDRLIBS = ./rtlsdr/lib/librtlsdr.a /usr/lib/x86_64-linux-gnu/libusb-1.0.so
SDRLIBS = /usr/local/lib/librtlsdr.so /usr/lib/x86_64-linux-gnu/libusb-1.0.so
# SDRFRAMEWORKS = #-framework foundation -framework iokit
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
