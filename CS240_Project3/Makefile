#------------------------------------------------------------#
# Name: Tom McArdle
# PUname: mcardle
#
# This is a Makefile for Project 3 - Directory Backup
#------------------------------------------------------------#


#------------- Constants ---------------------------------#

# .PHONY rule will run defined functions no matter what
.PHONY: target

# List of flags to pass to the compilation command
CFLAGS = -g

# CC is the C compiler to use
CC = gcc

# TOD0: Enter dependencies (.h and .c files)
HFILES = backup.h
CFILES = backup.c
DEPS = $(HFILES) $(CFILES)

# Compile 
COMPILE = $(CC) #-std=c89

# Run the Symbolic Target
RUNTARGET = ./$@

#------------- End Constants ---------------------------------#


#------------- Symbolic Targets ---------------------------------#

# Symbolic Targets:
all: target

target: backup.c
	$(COMPILE) -o $@ $^
	$(RUNTARGET) -s ./userData -d ./backupDir -m 3


#------------- End Symbolic Targets ---------------------------------#
