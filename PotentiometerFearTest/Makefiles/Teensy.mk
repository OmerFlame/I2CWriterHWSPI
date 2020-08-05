#
# embedXcode
# ----------------------------------
# Embedded Computing on Xcode
#
# Copyright © Rei Vilo, 2010-2020
# https://embedXcode.weebly.com
# All rights reserved
#
#
# Last update: 20 May 2020 release 11.7.19
#


ifeq ($(MAKEFILE_NAME),)

TEENSY_PATH     = $(TEENSY_APP)/Contents/Java
TEENSY_BOARDS   = $(TEENSY_PATH)/hardware/teensy/avr/boards.txt

#ifneq ($(call PARSE_FILE,$(BOARD_TAG),name,$(TEENSY_BOARDS)),)
#else ifneq ($(call PARSE_FILE,$(BOARD_TAG),name,$(GLOWDECK_BOARDS)),)

BOARD_CHECK    := 0
ifneq ($(call PARSE_FILE,$(BOARD_TAG),name,$(TEENSY_BOARDS)),)
    BOARD_CHECK    := 1
endif
ifneq ($(call PARSE_FILE,$(BOARD_TAG),name,$(GLOWDECK_BOARDS)),)
    BOARD_CHECK    := 1
endif

ifeq ($(BOARD_CHECK),1)
MAKEFILE_NAME = Teensy


# Teensy specifics
# ----------------------------------
#
PLATFORM         := Teensy
PLATFORM_TAG      = ARDUINO=10812 TEENSY_CORE EMBEDXCODE=$(RELEASE_NOW) ARDUINO_$(call PARSE_BOARD,$(BOARD_TAG),build.board)
APPLICATION_PATH := $(TEENSY_PATH)

t001 = $(APPLICATION_PATH)/lib/teensyduino.txt
t002 = $(APPLICATION_PATH)/lib/version.txt
MODIFIED_ARDUINO_VERSION =  $(shell if [ -f $(t002) ] ; then cat $(t002) ; fi)


# Release check
# ----------------------------------
#
    REQUIRED_TEENSY_RELEASE = 1.52
    TEENSY_RELEASE = $(shell if [ -f $(t001) ] ; then cat $(t001) ; fi)
    ifeq ($(shell if [[ '$(TEENSY_RELEASE)' > '$(REQUIRED_TEENSY_RELEASE)' ]] || [[ '$(TEENSY_RELEASE)' = '$(REQUIRED_TEENSY_RELEASE)' ]]; then echo 1 ; else echo 0 ; fi ),0)
        $(error Teensyduino release $(REQUIRED_TEENSY_RELEASE) or later is required, $(TEENSY_RELEASE) installed.)
endif
PLATFORM_VERSION := $(TEENSY_RELEASE) for Arduino $(MODIFIED_ARDUINO_VERSION)

# Complicated menu system for Arduino 1.5
#
# BOARD_TAGS_LIST includes all the BOARD_TAGs
# BOARD_OPTION_TAGS_LIST includes the BOARD_TAG options onmy
#
BOARD_OPTION_TAGS_LIST   = $(BOARD_TAG1) $(BOARD_TAG2) $(BOARD_TAG3) $(BOARD_TAG4)

SEARCH_FOR  = $(strip $(foreach t,$(1),$(call PARSE_BOARD,$(t),$(2))))

# Automatic Teensy2 or Teensy 3 selection based on build.core
#
BOARDS_TXT      := $(APPLICATION_PATH)/hardware/teensy/avr/boards.txt
BUILD_SUBCORE    = $(call PARSE_BOARD,$(BOARD_TAG),build.core)
#$(info BUILD_SUBCORE $(BUILD_SUBCORE))

DFLAGS = $(call PARSE_BOARD,$(BOARD_TAG),build.flags.defs)

ifeq ($(BUILD_SUBCORE),teensy)
    include $(MAKEFILE_PATH)/Teensy2.mk
else ifeq ($(BUILD_SUBCORE),teensy3)
    include $(MAKEFILE_PATH)/Teensy3.mk
else ifeq ($(BUILD_SUBCORE),teensy4)
	include $(MAKEFILE_PATH)/Teensy3.mk
else ifeq ($(BUILD_SUBCORE),glowdeck)
    include $(MAKEFILE_PATH)/Teensy3.mk
else
    $(error $(BUILD_SUBCORE) unknown) 
endif

# One single location for Teensyduino application libraries
# $(APPLICATION_PATH)/libraries aren't compatible
#
APP_LIB_PATH     := $(APPLICATION_PATH)/hardware/teensy/avr/libraries

a1000    = $(foreach dir,$(APP_LIB_PATH),$(patsubst %,$(dir)/%,$(APP_LIBS_LIST)))
a1000   += $(foreach dir,$(APP_LIB_PATH),$(patsubst %,$(dir)/%/utility,$(APP_LIBS_LIST)))
a1000   += $(foreach dir,$(APP_LIB_PATH),$(patsubst %,$(dir)/%/src,$(APP_LIBS_LIST)))
a1000   += $(foreach dir,$(APP_LIB_PATH),$(patsubst %,$(dir)/%/src/utility,$(APP_LIBS_LIST)))
a1000   += $(foreach dir,$(APP_LIB_PATH),$(patsubst %,$(dir)/%/src/arch/$(BUILD_SUBCORE),$(APP_LIBS_LIST)))
a1000   += $(foreach dir,$(APP_LIB_PATH),$(patsubst %,$(dir)/%/src/$(BUILD_SUBCORE),$(APP_LIBS_LIST)))

APP_LIB_CPP_SRC = $(foreach dir,$(a1000),$(wildcard $(dir)/*.cpp)) # */
APP_LIB_C_SRC   = $(foreach dir,$(a1000),$(wildcard $(dir)/*.c)) # */
APP_LIB_H_SRC   = $(foreach dir,$(a1000),$(wildcard $(dir)/*.h)) # */
APP_LIB_H_SRC  += $(foreach dir,$(a1000),$(wildcard $(dir)/*.hpp)) # */
APP_LIB_AS1_SRC = $(wildcard $(patsubst %,%/*.s,$(APP_LIBS))) # */
APP_LIB_AS2_SRC = $(wildcard $(patsubst %,%/*.S,$(APP_LIBS))) # */

APP_LIB_OBJS     = $(patsubst $(APPLICATION_PATH)/%.cpp,$(OBJDIR)/%.cpp.o,$(APP_LIB_CPP_SRC))
APP_LIB_OBJS    += $(patsubst $(APPLICATION_PATH)/%.c,$(OBJDIR)/%.c.o,$(APP_LIB_C_SRC))
APP_LIB_OBJS    += $(patsubst $(APPLICATION_PATH)/%.s,$(OBJDIR)/%.s.o,$(APP_LIB_AS1_SRC))
APP_LIB_OBJS    += $(patsubst $(APPLICATION_PATH)/%.S,$(OBJDIR)/%.S.o,$(APP_LIB_AS2_SRC))

BUILD_APP_LIBS_LIST = $(subst $(BUILD_APP_LIB_PATH)/, ,$(APP_LIB_CPP_SRC))

USB_VID   := $(call PARSE_BOARD,$(BOARD_TAG),build.vid)
USB_PID   := $(call PARSE_BOARD,$(BOARD_TAG),build.pid)

ifneq ($(USB_PID),)
ifneq ($(USB_VID),)
    USB_FLAGS  = -DUSB_VID=$(USB_VID)
    USB_FLAGS += -DUSB_PID=$(USB_PID)
endif
endif

ifeq ($(USB_FLAGS),)
    USB_FLAGS = -DUSB_VID=null -DUSB_PID=null
endif

USB_FLAGS += -DUSB_SERIAL -DLAYOUT_US_ENGLISH -DTIME_T=$(shell date +%s)

MAX_RAM_SIZE = $(call PARSE_BOARD,$(BOARD_TAG),upload.maximum_ram_size)

# Specific OBJCOPYFLAGS for objcopy only
# objcopy uses OBJCOPYFLAGS only
#
OBJCOPYFLAGS  = -R .eeprom -O ihex

# Target
#
TARGET_HEXBIN    = $(TARGET_HEX)

# Copy command
#
COMMAND_COPY     = $(OBJCOPY) -O ihex -R .eeprom $< $@

# Link command
#
COMMAND_LINK    = $(CC) $(OUT_PREPOSITION)$@ $(LOCAL_OBJS) $(TARGET_A) $(LDFLAGS)

endif

endif

