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
# Last update: 03 May 2019 release 10.8.3
#
# For GCC ELF and large model, see EnergiaMSP430ELF_19.mk
#


ifeq ($(MAKEFILE_NAME),)

ENERGIA_MSP430_1    = $(ENERGIA_PACKAGES_PATH)/hardware/msp430/$(ENERGIA_MSP430_RELEASE)

ifneq ($(wildcard $(ENERGIA_MSP430_1)),)
    ENERGIA_MSP430_APP    = $(ENERGIA_MSP430_1)
    ENERGIA_MSP430_PATH   = $(ENERGIA_PACKAGES_PATH)
    ENERGIA_19_MSP430_BOARDS = $(ENERGIA_MSP430_1)/boards.txt
endif

ifneq ($(call PARSE_FILE,$(BOARD_TAG_18),name,$(ENERGIA_19_MSP430_BOARDS)),)
MAKEFILE_NAME = EnergiaMSP430_19


# Energia LaunchPad MSP430 and FR5739 specifics
# ----------------------------------
#
PLATFORM         := Energia
BUILD_CORE       := msp430
SUB_PLATFORM     := msp430


APPLICATION_PATH := $(ENERGIA_18_PATH)

ifneq ($(ENERGIA_MSP430ELF_GCC_RELEASE),)
ifneq ($(wildcard $(ENERGIA_PACKAGES_PATH)/tools/msp430-gcc/$(ENERGIA_MSP430ELF_GCC_RELEASE)),)
    ENERGIA_MSP430_GCC_RELEASE := $(ENERGIA_MSP430ELF_GCC_RELEASE)
    $(info GCC MSP set to $(ENERGIA_MSP430ELF_GCC_RELEASE))
endif
endif

#ENERGIA_RELEASE := $(shell tail -c2 $(APPLICATION_PATH)/lib/version.txt)
#ARDUINO_RELEASE := $(shell head -c4 $(APPLICATION_PATH)/lib/version.txt | tail -c3)
ENERGIA_RELEASE   := 10807
ARDUINO_RELEASE   := 10807

PLATFORM_VERSION := MSP430 $(ENERGIA_MSP430_RELEASE) for Energia $(ENERGIA_RELEASE)
BOARD_TAG        := $(BOARD_TAG_18)

HARDWARE_PATH     = $(ENERGIA_MSP430_PATH)/hardware/msp430/$(ENERGIA_MSP430_RELEASE)
TOOL_CHAIN_PATH   := $(ENERGIA_PACKAGES_PATH)/tools/msp430-gcc/$(ENERGIA_MSP430_GCC_RELEASE)/bin
OTHER_TOOLS_PATH  = $(ENERGIA_PACKAGES_PATH)/tools/DSLite/$(ENERGIA_MSP430_DSLITE_RELEASE)

#PLATFORM_TAG      = ENERGIA=$(ENERGIA_RELEASE) ARDUINO=$(ARDUINO_RELEASE) EMBEDXCODE=$(RELEASE_NOW) ENERGIA_ARCH_MSP430 ENERGIA_$(BOARD_TAG) $(filter __%__ ,$(GCC_PREPROCESSOR_DEFINITIONS))
PLATFORM_TAG      = ENERGIA=$(ENERGIA_RELEASE) ARDUINO=$(ARDUINO_RELEASE) EMBEDXCODE=$(RELEASE_NOW) ENERGIA_ARCH_MSP430 ENERGIA_$(call PARSE_BOARD,$(BOARD_TAG),build.board) $(filter __%__ ,$(GCC_PREPROCESSOR_DEFINITIONS))


# Uploader
# ----------------------------------
#
ifeq ($(UPLOADER), mspdebug)
    # mspdebug for MSP-EXP430G2553LP and MSP-EXP430FR5739LP only
    UPLOADER          = mspdebug
#    UPLOADER_PATH     = $(APPLICATION_PATH)/hardware/tools/msp430/bin
    UPLOADER_PATH     = $(ENERGIA_MSP430_PATH)/tools/mspdebug/$(ENERGIA_MSPDEBUG_RELEASE)
    UPLOADER_EXEC     = $(UPLOADER_PATH)/mspdebug
    UPLOADER_PROTOCOL = $(call PARSE_BOARD,$(BOARD_TAG),upload.protocol)
    UPLOADER_OPTS     = $(UPLOADER_PROTOCOL) --force-reset

    # MSP-EXP430FR5739LP requires a specific command
    #
    ifeq ($(BOARD_TAG), MSP-EXP430FR5739LP)
        UPLOADER_COMMAND = load
    else
        UPLOADER_COMMAND = prog
    endif
else
    # General case with DSLite
    UPLOADER          = DSLite
    UPLOADER_PATH     = $(OTHER_TOOLS_PATH)/DebugServer/bin
    UPLOADER_EXEC     = $(UPLOADER_PATH)/DSLite
    UPLOADER_OPTS     = $(OTHER_TOOLS_PATH)/$(VARIANT).ccxml

    COMMAND_UPLOAD = $(UPLOADER_EXEC) load -c $(UPLOADER_OPTS) -f $(TARGET_ELF)
endif

CORE_LIB_PATH    := $(HARDWARE_PATH)/cores/msp430
APP_LIB_PATH     := $(HARDWARE_PATH)/libraries
BOARDS_TXT       := $(HARDWARE_PATH)/boards.txt
#BOARDS_TXT      := $(APPLICATION_PATH)/hardware/msp430/boards.txt

# Sketchbook/Libraries path
# ----------------------------------
# wildcard required for ~ management
# ?ibraries required for libraries and Libraries
#
ifeq ($(USER_LIBRARY_DIR)/Energia15/preferences.txt,)
    $(error Error: run Energia once and define the sketchbook path)
endif

ifeq ($(shell if [ -d '$(SKETCHBOOK_DIR)' ]; then echo 1 ; fi ),)
    SKETCHBOOK_DIR = $(shell grep sketchbook.path $(wildcard ~/Library/Energia15/preferences.txt) | cut -d = -f 2)
endif

ifeq ($(shell if [ -d '$(SKETCHBOOK_DIR)' ]; then echo 1 ; fi ),)
    $(error Error: sketchbook path not found)
endif
USER_LIB_PATH  = $(wildcard $(SKETCHBOOK_DIR)/?ibraries)


# Horrible patch for core libraries
# ----------------------------------
#
# If driverlib/libdriverlib.a is available, exclude driverlib/
#
CORE_LIB_PATH   = $(HARDWARE_PATH)/cores/msp430
CORE_LIB_PATH  += $(HARDWARE_PATH)/cores/msp430/avr

#CORE_A   = $(CORE_LIB_PATH)/driverlib/libdriverlib.a
#
#BUILD_CORE_LIB_PATH = $(shell find $(CORE_LIB_PATH) -type d)
#ifneq ($(wildcard $(CORE_A)),)
#    BUILD_CORE_LIB_PATH := $(filter-out %/driverlib,$(BUILD_CORE_LIB_PATH))
#endif

BUILD_CORE_CPP_SRCS = $(filter-out %program.cpp %main.cpp,$(foreach dir,$(CORE_LIB_PATH),$(wildcard $(dir)/*.cpp))) # */
BUILD_CORE_C_SRCS   = $(foreach dir,$(CORE_LIB_PATH),$(wildcard $(dir)/*.c)) # */

BUILD_CORE_OBJ_FILES  = $(BUILD_CORE_C_SRCS:.c=.c.o) $(BUILD_CORE_CPP_SRCS:.cpp=.cpp.o)
BUILD_CORE_OBJS       = $(patsubst $(HARDWARE_PATH)/%,$(OBJDIR)/%,$(BUILD_CORE_OBJ_FILES))

CORE_LIBS_LOCK = 1
# ----------------------------------


# Horrible patch for Ethernet library
# ----------------------------------
#
# APPlication Arduino/chipKIT/Digispark/Energia/Maple/Microduino/Teensy/Wiring sources
#
APP_LIB_PATH     := $(HARDWARE_PATH)/libraries

ifneq ($(strip $(APP_LIBS_LIST)),0)
msp430_20    = $(foreach dir,$(APP_LIB_PATH),$(patsubst %,$(dir)/%,$(APP_LIBS_LIST)))
msp430_20   += $(foreach dir,$(APP_LIB_PATH),$(patsubst %,$(dir)/%/utility,$(APP_LIBS_LIST)))

APP_LIB_CPP_SRC = $(foreach dir,$(msp430_20),$(wildcard $(dir)/*.cpp)) # */
APP_LIB_C_SRC   = $(foreach dir,$(msp430_20),$(wildcard $(dir)/*.c)) # */
APP_LIB_H_SRC   = $(foreach dir,$(msp430_20),$(wildcard $(dir)/*.h)) # */

APP_LIB_OBJS     = $(patsubst $(HARDWARE_PATH)/%.cpp,$(OBJDIR)/%.cpp.o,$(APP_LIB_CPP_SRC))
APP_LIB_OBJS    += $(patsubst $(HARDWARE_PATH)/%.c,$(OBJDIR)/%.c.o,$(APP_LIB_C_SRC))

BUILD_APP_LIBS_LIST = $(subst $(APP_LIB_PATH)/, ,$(APP_LIB_CPP_SRC))
BUILD_APP_LIB_PATH  = $(msp430_20) $(foreach dir,$(APP_LIB_PATH),$(patsubst %,$(dir)/%,$(APP_LIBS_LIST)))
endif

BUILD_APP_LIB_PATH     = $(APPLICATION_PATH)/libraries

msp430_10    = $(foreach dir,$(BUILD_APP_LIB_PATH),$(patsubst %,$(dir)/%,$(APP_LIBS_LIST)))
msp430_10   += $(foreach dir,$(BUILD_APP_LIB_PATH),$(patsubst %,$(dir)/%/utility,$(APP_LIBS_LIST)))
msp430_10   += $(foreach dir,$(BUILD_APP_LIB_PATH),$(patsubst %,$(dir)/%/src,$(APP_LIBS_LIST)))
msp430_10   += $(foreach dir,$(BUILD_APP_LIB_PATH),$(patsubst %,$(dir)/%/src/utility,$(APP_LIBS_LIST)))
msp430_10   += $(foreach dir,$(BUILD_APP_LIB_PATH),$(patsubst %,$(dir)/%/src/arch/$(BUILD_CORE),$(APP_LIBS_LIST)))

BUILD_APP_LIB_CPP_SRC = $(foreach dir,$(msp430_10),$(wildcard $(dir)/*.cpp)) # */
BUILD_APP_LIB_C_SRC   = $(foreach dir,$(msp430_10),$(wildcard $(dir)/*.c)) # */
BUILD_APP_LIB_H_SRC   = $(foreach dir,$(msp430_10),$(wildcard $(dir)/*.h)) # */

BUILD_APP_LIB_OBJS     = $(patsubst $(APPLICATION_PATH)/%.cpp,$(OBJDIR)/%.cpp.o,$(BUILD_APP_LIB_CPP_SRC))
BUILD_APP_LIB_OBJS    += $(patsubst $(APPLICATION_PATH)/%.c,$(OBJDIR)/%.c.o,$(BUILD_APP_LIB_C_SRC))

APP_LIBS_LOCK = 1
# ----------------------------------


# Release 4.6.5 uses msp430-gcc
# Release 6.4.0.32 uses msp430-elf-gcc
# Release 7.3.1.24 uses msp430-elf-gcc
# Tool-chain names
#
ifeq ($(ENERGIA_MSP430_GCC_RELEASE),6.4.0.32)
    GCC_PREFIX = msp430-elf
else ifeq ($(ENERGIA_MSP430_GCC_RELEASE),7.3.1.24)
    GCC_PREFIX = msp430-elf
    LARGE_OPTIONS = $(call PARSE_BOARD,$(BOARD_TAG),build.extra_flags)

    ifeq ($(LARGE_OPTIONS),)
        LARGE_OPTIONS += -mlarge -mcode-region=upper -mhwmult=auto
    endif
    COMPILER_OPTIONS += $(LARGE_OPTIONS) -gdwarf-3 -gstrict-dwarf

    WARNING_MESSAGE = Support for MSP GCC $(ENERGIA_MSP430_GCC_RELEASE) is experimental.
else
    GCC_PREFIX = msp430
endif

CC      = $(TOOL_CHAIN_PATH)/$(GCC_PREFIX)-gcc
CXX     = $(TOOL_CHAIN_PATH)/$(GCC_PREFIX)-g++
AR      = $(TOOL_CHAIN_PATH)/$(GCC_PREFIX)-ar
OBJDUMP = $(TOOL_CHAIN_PATH)/$(GCC_PREFIX)-objdump
OBJCOPY = $(TOOL_CHAIN_PATH)/$(GCC_PREFIX)-objcopy
SIZE    = $(TOOL_CHAIN_PATH)/$(GCC_PREFIX)-size
NM      = $(TOOL_CHAIN_PATH)/$(GCC_PREFIX)-nm

BOARD          = $(call PARSE_BOARD,$(BOARD_TAG),board)
#LDSCRIPT = $(call PARSE_BOARD,$(BOARD_TAG),ldscript)
VARIANT        = $(call PARSE_BOARD,$(BOARD_TAG),build.variant)
VARIANT_PATH   = $(HARDWARE_PATH)/variants/$(VARIANT)

OPTIMISATION   = -Os

MCU_FLAG_NAME  = mmcu
MCU     = $(call PARSE_BOARD,$(BOARD_TAG),build.mcu)
F_CPU   = $(call PARSE_BOARD,$(BOARD_TAG),build.f_cpu)

INCLUDE_PATH    := $(VARIANT_PATH)
INCLUDE_PATH    += $(CORE_LIB_PATH)
INCLUDE_PATH    += $(BUILD_CORE_LIB_PATH)
INCLUDE_PATH    += $(APP_LIB_PATH)
INCLUDE_PATH    += $(BUILD_APP_LIB_PATH)
INCLUDE_PATH    += $(sort $(dir $(APP_LIB_H_SRC) $(BUILD_APP_LIB_H_SRC)))
INCLUDE_PATH    += $(ENERGIA_PACKAGES_PATH)/tools/msp430-gcc/$(ENERGIA_MSP430_GCC_RELEASE)/include

INCLUDE_LIBS    += $(ENERGIA_PACKAGES_PATH)/tools/msp430-gcc/$(ENERGIA_MSP430_GCC_RELEASE)/include


# Flags for gcc, g++ and linker
# ----------------------------------
#
# Common CPPFLAGS for gcc, g++, assembler and linker
#
CPPFLAGS     = $(OPTIMISATION) $(WARNING_FLAGS)
CPPFLAGS    += -c -Os -Wall -ffunction-sections -fdata-sections
CPPFLAGS    += -$(MCU_FLAG_NAME)=$(MCU) -DF_CPU=$(F_CPU)
CPPFLAGS    += $(addprefix -I, $(INCLUDE_PATH))
CPPFLAGS    += $(addprefix -D, $(PLATFORM_TAG))

# Specific CFLAGS for gcc only
# gcc uses CPPFLAGS and CFLAGS
#
CFLAGS       = #

# Specific CXXFLAGS for g++ only
# g++ uses CPPFLAGS and CXXFLAGS
#
CXXFLAGS    = -fno-exceptions -fno-threadsafe-statics -fno-rtti

# Specific ASFLAGS for gcc assembler only
# gcc assembler uses CPPFLAGS and ASFLAGS
#
ASFLAGS      = --asm_extension=S

# Specific LDFLAGS for linker only
# linker uses CPPFLAGS and LDFLAGS
#
LDFLAGS     := $(OPTIMISATION) $(WARNING_FLAGS)
LDFLAGS     += -fno-rtti -fno-exceptions -Wl,--gc-sections,-u,main 
LDFLAGS     += -$(MCU_FLAG_NAME)=$(MCU) -DF_CPU=$(F_CPU)
LDFLAGS     += $(addprefix -I, $(INCLUDE_PATH))
LDFLAGS     += $(addprefix -L, $(INCLUDE_LIBS))
LDFLAGS     += -LBuilds -lm

ifeq ($(ENERGIA_MSP430_GCC_RELEASE),7.3.1.24)
    LDFLAGS     += -T $(MCU).ld
endif

# Specific OBJCOPYFLAGS for objcopy only
# objcopy uses OBJCOPYFLAGS only
#
OBJCOPYFLAGS  = -Oihex


# Target
#
TARGET_HEXBIN = $(TARGET_HEX)


# Commands
# ----------------------------------
#
# Link command
#
COMMAND_LINK    = $(CC) $(OUT_PREPOSITION)$@ $(LOCAL_OBJS) $(TARGET_A) $(LDFLAGS)

endif

endif

