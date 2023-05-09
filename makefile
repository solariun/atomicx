#//               GNU GENERAL PUBLIC LICENSE
#//                Version 3, 29 June 2007
#//
#//Copyright (C) 2007 Free Software Foundation, Inc. <https://fsf.org/>
#//Everyone is permitted to copy and distribute verbatim copies
#//of this license document, but changing it is not allowed.
#//
#//Preamble
#//
#//The GNU General Public License is a free, copyleft license for
#//software and other kinds of works.
#//
#//The licenses for most software and other practical works are designed
#//to take away your freedom to share and change the works.  By contrast,
#//the GNU General Public License is intended to guarantee your freedom to
#//share and change all versions of a program--to make sure it remains free
#//software for all its users.  We, the Free Software Foundation, use the
#//GNU General Public License for most of our software; it applies also to
#//any other work released this way by its authors.  You can apply it to
#//your programs, too.
#//
#// See LICENSE file for the complete information

#
# 'make'        build executable file 'atomicx.bin'
# 'make clean'  removes all .o and executable files
#
# use EXTRA_FLAGS=_DEBUG=<TRACE, DEBUG, INFO. WARNING. ERROR, CRITICAL>
# .   for logging

# define the C compiler to use
CC = g++

# define any compile-time flags
CFLAGS = -Ofast -Wall -g --std=c++11 -Wall -Wextra #-Werror

CPX_DIR ?= ./source
TEST_DIR ?= ./test
BIN_DIR ?= ./bin

# define any directories containing header files other than /usr/include
#
INCLUDES = -I$(CPX_DIR)

# define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath, something like:
# LFLAGS = -L/home/newhall/lib  -L../lib

# define any libraries to link into executable:
#   if I want to link in libraries (libx.so or libx.a) I use the -llibname
#   option, something like (this will link in libmylib.so and libm.so:
#LIBS = -lmylib -lm

# define the C source files
SRCS = $(wildcard $(TEST_DIR)/*.cpp) $(wildcard $(CPX_DIR)/*.cpp)

# define the C object files
#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .c of all words in the macro SRCS
# with the .o suffix
#
OBJS = $(SRCS:.cpp=.o)

# define the executable file
TARGET = $(BIN_DIR)/atomix.bin

#
# The following part of the makefile is generic; it can be used to
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make` depend'
#

define FUNC_MAKE_DIR
	echo "Bin diretory: [$(1)]"; \
	if [ ! -d "$(1)" ]; \
	then \
		if mkdir -p "$(1)"; \
		then \
			echo "Bin directory generated: $(1)"; \
		else \
			echo "Error, could not create a $(1)"; \
			exit 1; \
		fi; \
	fi
endef

TEST_DIR ?= test

PROJECT ?= test

SOURCE_DIR ?= $(TEST_DIR)/$(PROJECT)

.PHONY: build

# same as all:
# 	Making multiple targets and you want all of them to run? Make an all target.
# *Since this is the first rule listed*, it will run by default if make is called without
# specifying a target, regardless the name, "all" is a convention name for it.
default: build
	@echo  AtomicX binary $(TARGET) has been compiled
	@echo executing $(TARGET)
	$(TARGET)

pc: clean default

makedir:
	$(call FUNC_MAKE_DIR,$(BIN_DIR))
	@echo "OBJs: [$(OBJS)]"

build: makedir $(TARGET)

lldb: clean build
	@echo  AtomicX binary $(TARGET) has been compiled
	@echo starting lldb $(TARGET)
	lldb $(TARGET)

gdb: clean build
	@echo  AtomicX binary $(TARGET) has been compiled
	@echo starting lldb $(TARGET)
	gdb $(TARGET)

# ------------------------------
# Arduino project, use variable PROJECT=name 
# to select which project to compile and flash
# if not supplied it will use test/test as default.
# ------------------------------

nano:
	arduino-cli compile -b arduino:avr:nano:cpu=atmega328 --build-property build.extra_flags='$(EXTRA_FLAGS)' --upload -P usbasp  "$(SOURCE_DIR)"

esp8266:
	arduino-cli compile -v -b esp8266:esp8266:nodemcuv2:baud=921600 --build-property build.extra_flags='$(EXTRA_FLAGS)' -upload -p "$(serial)" "$(SOURCE_DIR)"

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET) $(OBJS) $(LFLAGS) $(LIBS)

# this is a suffix replacement rule for building .o's from .cpp's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .cpp file) and $@: the name of the target of the rule (a .o file)
# (see the gnu make manual section about automatic variables)
.cpp.o:
	$(CC) $(CFLAGS) $(EXTRA_FLAGS) $(INCLUDES) -c $<  -o $@

clean:
	@echo "CLEANING: $(OBJ) $(TARGET) *~ "
	$(RM) $(OBJS) *~ $(TARGET)

document:
	@echo  AtomicX Generating documents
	Doxygen
