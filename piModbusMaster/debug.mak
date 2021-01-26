#Generated by VisualGDB (http://visualgdb.com)
#DO NOT EDIT THIS FILE MANUALLY UNLESS YOU ABSOLUTELY NEED TO
#USE VISUALGDB PROJECT PROPERTIES DIALOG INSTEAD

BINARYDIR := Debug

#Toolchain
CC := $(TOOLCHAIN_ROOT)/bin/arm-linux-gnueabihf-gcc.exe
CXX := $(TOOLCHAIN_ROOT)/bin/arm-linux-gnueabihf-g++.exe
LD := $(CXX)
AR := $(TOOLCHAIN_ROOT)/bin/arm-linux-gnueabihf-ar.exe
OBJCOPY := $(TOOLCHAIN_ROOT)/bin/arm-linux-gnueabihf-objcopy.exe

#Additional flags
PREPROCESSOR_MACROS := DEBUG=1 _XOPEN_SOURCE __KUNBUSPI__
INCLUDE_DIRS := . ../../../platformFbus/compiler/sw ../../../platformFbus/common/sw ../../../platformFbus/compiler/sw/gnuArm ../../../platform/sw/piControl ../src/piConfigParser ../src
LIBRARY_DIRS := 
LIBRARY_NAMES := modbus rt pthread json-c
ADDITIONAL_LINKER_INPUTS := 
MACOS_FRAMEWORKS := 
LINUX_PACKAGES := 

CFLAGS := -ggdb -ffunction-sections -O0
CXXFLAGS := -ggdb -ffunction-sections -O0
ASFLAGS := 
LDFLAGS := -Wl,-gc-sections
COMMONFLAGS := -std=c99 -Wall -Wextra -pedantic
LINKER_SCRIPT := 

START_GROUP := -Wl,--start-group
END_GROUP := -Wl,--end-group

#Additional options detected from testing the toolchain
USE_DEL_TO_CLEAN := 1
CP_NOT_AVAILABLE := 1
IS_LINUX_PROJECT := 1
