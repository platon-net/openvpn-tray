#
# Makefile
#
# Developed by Ondrej Jombik <nepto@platon.sk>
# Copyright (c) 2024 Platon Group, http://platon.sk/
# Licensed under terms of GNU General Public License.
# All rights reserved.
#
# Changelog:
# 2024-10-17 - created
#

# $Platon$ 

# Makefile for creating distribution of test package
# Type 'make dist' for create tar-gziped archiv. 

PACKAGE = test
VERSION = 1.0

# Compiler and flags
CC = gcc
CFLAGS = `pkg-config --cflags gtk+-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0`

# Files
SRC = openvpn-tray.c
RES_XML = resources.xml
RES_SRC = resources.c
RES_GRESOURCE = resources.gresource
OUTPUT = openvpn-tray

# Resource files (PNG images)
IMAGES = images/openvpn-on.png images/openvpn-off.png

# Build targets
all: $(OUTPUT)

# Compile the GResource source from the XML definition
$(RES_SRC): $(RES_XML) $(IMAGES)
	glib-compile-resources $(RES_XML) --target=$(RES_SRC) --generate-source

# Compile the GResource binary file
$(RES_GRESOURCE): $(RES_XML) $(IMAGES)
	glib-compile-resources $(RES_XML) --target=$(RES_GRESOURCE)

# Compile the application including the GResource file
$(OUTPUT): $(SRC) $(RES_SRC)
	$(CC) $(CFLAGS) $(SRC) $(RES_SRC) -o $(OUTPUT) $(LDFLAGS)

# Clean up compiled files
clean:
	rm -f $(OUTPUT) $(RES_SRC) $(RES_GRESOURCE)

# vim600: fdm=marker fdc=3

