NAME=rtlsdr~
CSYM=rtlsdr_tilde


PDINCLUDE =  -I../../src
# these were just for testing
LAMA =
LAMO =

# rtlsdr stuff

SDRLIBINC = -I/opt/local/include/libusb-1.0 -I../include
SDRLIBS = /usr/local/lib/libusb32-1.0.a ./rtlsdr/lib/librtlsdr32.a
SDRFRAMEWORKS = -framework foundation -framework iokit
SDRINC = -I./rtlsdr/include
SDRSRC = ./rtlsdr/*.c
SDROBJ = rtl_fm3.o convenience.o

# ----------------------- Mac OSX -----------------------

d_pd_darwin: $(NAME).pd_darwin

.SUFFIXES: .pd_darwin

# compile with debugging information
DARWINCFLAGS = -DPD -g -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-unused-parameter -Wno-parentheses -Wno-switch


.c.pd_darwin:

	$(CC) -arch i386  $(DARWINCFLAGS) $(PDINCLUDE) $(SDRLIBINC) $(SDRINC) -c $*.c $(SDRSRC) $(LAMA)

#	$(CC) -arch i386  -bundle -undefined suppress -flat_namespace \
				    -o $*.pd_darwin $*.o $(LAMO) /usr/local/lib/libusb.dylib /usr/local/lib/libftdi.dylibå
	$(CC) -arch i386  -bundle -undefined suppress -flat_namespace \
					-o $*.pd_darwin $*.o $(SDROBJ) $(LAMO) $(SDRLIBS) $(SDRFRAMEWORKS)
	rm -f $*.o 

# ----------------------------------------------------------

clean:
	rm -f *.o *.pd_* so_locations
