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
# Last update: 20 Feb 2019 release 10.5.7
#


ifeq ($(MAKEFILE_NAME),)

# nRF5 boards path for Arduino 1.8.0
#
NRF5_BOARDS_1    = $(ARDUINO_180_PACKAGES_PATH)/sandeepmistry

ifneq ($(wildcard $(NRF5_BOARDS_1)),)
    NRF5_BOARDS_APP     = $(NRF5_BOARDS_1)
    NRF5_BOARDS_PATH    = $(NRF5_BOARDS_APP)
    NRF5_BOARDS_BOARDS  = $(NRF5_BOARDS_APP)/hardware/nRF5/$(NRF5_BOARDS_RELEASE)/boards.txt
endif

ifneq ($(call PARSE_FILE,$(BOARD_TAG),name,$(NRF5_BOARDS_BOARDS)),)
MAKEFILE_NAME = nRF51_Boards_180


# nRF5 Boards specifics
# ----------------------------------
#
PLATFORM         := sandeepmistry
BUILD_CORE        = $(call PARSE_BOARD,$(BOARD_TAG),build.core)
SUB_PLATFORM      = $(BUILD_CORE)
PLATFORM_TAG      = EMBEDXCODE=$(RELEASE_NOW) ARDUINO=10812
APPLICATION_PATH := $(NRF5_BOARDS_PATH)
PLATFORM_VERSION := $(NRF5_BOARDS_RELEASE) for Arduino $(ARDUINO_IDE_RELEASE)

HARDWARE_PATH     = $(APPLICATION_PATH)/hardware/nRF5/$(NRF5_BOARDS_RELEASE)
TOOL_CHAIN_PATH   = $(APPLICATION_PATH)/tools/gcc-arm-none-eabi/$(NRF5_GCC_ARM_RELEASE)
OTHER_TOOLS_PATH  = $(APPLICATION_PATH)/tools

BUILD_CORE       = nrf52
SUB_PLATFORM     = nrf52
BOARDS_TXT      := $(HARDWARE_PATH)/boards.txt


# Release check
# ----------------------------------
#
REQUIRED_NRF5_BOARDS_RELEASE = 0.6.0
ifeq ($(shell if [[ '$(NRF5_BOARDS_RELEASE)' > '$(REQUIRED_NRF5_BOARDS_RELEASE)' ]] || [[ '$(NRF5_BOARDS_RELEASE)' = '$(REQUIRED_NRF5_BOARDS_RELEASE)' ]]; then echo 1 ; else echo 0 ; fi ),0)
$(error nRF5 Boards release $(REQUIRED_NRF5_BOARDS_RELEASE) or later required, release $(NRF5_BOARDS_RELEASE) installed)
endif

# Complicated menu system for Arduino 1.5
# Another example of Arduino's quick and dirty job
# BOARD_TAG1 mentions the SoftService version
#
BOARD_TAGS_LIST   = $(BOARD_TAG) $(BOARD_TAG1) $(BOARD_TAG2) $(BOARD_TAG3) $(BOARD_TAG4)

SEARCH_FOR  = $(strip $(foreach t,$(1),$(call PARSE_BOARD,$(t),$(2))))


# Uploader
# ----------------------------------
#
# Uploader openocd
# UPLOADER defined in .xcconfig
#
UPLOADER         = openocd
UPLOADER_PATH    = $(OTHER_TOOLS_PATH)/openocd/0.10.0-dev.nrf5
UPLOADER_EXEC    = $(UPLOADER_PATH)/bin/openocd
UPLOADER_OPTS    = -d2 -f interface/cmsis-dap.cfg
#UPLOADER_OPTS   += -c \"transport select swd;\"
#UPLOADER_OPTS   += -f target/nrf51.cfg
UPLOADER_OPTS   += -f target/$(call PARSE_BOARD,$(BOARD_TAG),upload.target).cfg
UPLOADER_COMMAND = program {{$(TARGET_HEX)}} verify reset; shutdown;
# telnet_port disabled;
COMMAND_UPLOAD   = $(UPLOADER_EXEC) $(UPLOADER_OPTS) -c "$(UPLOADER_COMMAND)"


APP_TOOLS_PATH   := $(TOOL_CHAIN_PATH)/bin
CORE_LIB_PATH    := $(HARDWARE_PATH)/cores/NRF5
APP_LIB_PATH     := $(HARDWARE_PATH)/libraries


# Core files
# Crazy maze of sub-folders
#
CORE_C_SRCS          = $(shell find $(CORE_LIB_PATH) -name \*.c)
nrf5boards1300       = $(filter-out %main.cpp, $(shell find $(CORE_LIB_PATH) -name \*.cpp))
CORE_CPP_SRCS        = $(filter-out %/$(EXCLUDE_LIST),$(nrf5boards1300))
CORE_AS1_SRCS        = $(shell find $(CORE_LIB_PATH) -name \*.S)
CORE_AS1_SRCS_OBJ    = $(patsubst %.S,%.S.o,$(filter %.S, $(CORE_AS1_SRCS)))
CORE_AS2_SRCS        = $(shell find $(CORE_LIB_PATH) -name \*.s)
CORE_AS2_SRCS_OBJ    = $(patsubst %.s,%.s.o,$(filter %.s, $(CORE_AS_SRCS)))

CORE_OBJ_FILES       = $(CORE_C_SRCS:.c=.c.o) $(CORE_CPP_SRCS:.cpp=.cpp.o) $(CORE_AS1_SRCS_OBJ) $(CORE_AS2_SRCS_OBJ)
CORE_OBJS            = $(patsubst $(HARDWARE_PATH)/%,$(OBJDIR)/%,$(CORE_OBJ_FILES))

CORE_LIBS_LOCK       = 1

#ifeq ($(strip $(APP_LIBS_LIST)),0)
#    APP_LIBS_LIST        = Bluefruit52Lib nffs
#else
#    APP_LIBS_LIST       += Bluefruit52Lib nffs
#endif

# Two locations for libraries
# First from package
#
APP_LIB_PATH     := $(HARDWARE_PATH)/libraries

nrf5boards1000    = $(foreach dir,$(APP_LIB_PATH),$(patsubst %,$(dir)/%,$(APP_LIBS_LIST)))
nrf5boards1000   += $(foreach dir,$(APP_LIB_PATH),$(patsubst %,$(dir)/%/utility,$(APP_LIBS_LIST)))
nrf5boards1000   += $(foreach dir,$(APP_LIB_PATH),$(patsubst %,$(dir)/%/src,$(APP_LIBS_LIST)))
nrf5boards1000   += $(foreach dir,$(APP_LIB_PATH),$(patsubst %,$(dir)/%/src/utility,$(APP_LIBS_LIST)))
nrf5boards1000   += $(foreach dir,$(APP_LIB_PATH),$(patsubst %,$(dir)/%/src/service,$(APP_LIBS_LIST)))
nrf5boards1000   += $(foreach dir,$(APP_LIB_PATH),$(patsubst %,$(dir)/%/src/arch/$(BUILD_CORE),$(APP_LIBS_LIST)))
nrf5boards1000   += $(foreach dir,$(APP_LIB_PATH),$(patsubst %,$(dir)/%/src/$(BUILD_CORE),$(APP_LIBS_LIST)))

# Two specific libraries
#
ifneq ($(filter Bluefruit52Lib,$(APP_LIBS_LIST)),)
    nrf5boards1000b   += $(shell find $(APP_LIB_PATH)/Bluefruit52Lib/src -type d)
    nrf5boards1000    += $(nrf5boards1000b)
endif
ifneq ($(filter nffs,$(APP_LIBS_LIST)),)
    nrf5boards1000c   += $(shell find $(APP_LIB_PATH)/FileSystem/src -type d)
    nrf5boards1000    += $(nrf5boards1000c)
endif

APP_LIB_CPP_SRC = $(foreach dir,$(nrf5boards1000),$(wildcard $(dir)/*.cpp)) # */
APP_LIB_C_SRC   = $(foreach dir,$(nrf5boards1000),$(wildcard $(dir)/*.c)) # */
APP_LIB_S_SRC   = $(foreach dir,$(nrf5boards1000),$(wildcard $(dir)/*.S)) # */
APP_LIB_H_SRC   = $(foreach dir,$(nrf5boards1000),$(wildcard $(dir)/*.h)) # */
APP_LIB_H_SRC  += $(foreach dir,$(nrf5boards1000),$(wildcard $(dir)/*.hpp)) # */

APP_LIB_OBJS     = $(patsubst $(HARDWARE_PATH)/%.cpp,$(OBJDIR)/%.cpp.o,$(APP_LIB_CPP_SRC))
APP_LIB_OBJS    += $(patsubst $(HARDWARE_PATH)/%.c,$(OBJDIR)/%.c.o,$(APP_LIB_C_SRC))

BUILD_APP_LIBS_LIST = $(subst $(BUILD_APP_LIB_PATH)/, ,$(APP_LIB_CPP_SRC))

# Second from Arduino.CC
#
BUILD_APP_LIB_PATH     = $(APPLICATION_PATH)/libraries

nrf5boards1100    = $(foreach dir,$(BUILD_APP_LIB_PATH),$(patsubst %,$(dir)/%,$(APP_LIBS_LIST)))
nrf5boards1100   += $(foreach dir,$(BUILD_APP_LIB_PATH),$(patsubst %,$(dir)/%/utility,$(APP_LIBS_LIST)))
nrf5boards1100   += $(foreach dir,$(BUILD_APP_LIB_PATH),$(patsubst %,$(dir)/%/src,$(APP_LIBS_LIST)))
nrf5boards1100   += $(foreach dir,$(BUILD_APP_LIB_PATH),$(patsubst %,$(dir)/%/src/utility,$(APP_LIBS_LIST)))
nrf5boards1100   += $(foreach dir,$(BUILD_APP_LIB_PATH),$(patsubst %,$(dir)/%/src/arch/$(BUILD_CORE),$(APP_LIBS_LIST)))
nrf5boards1100   += $(foreach dir,$(BUILD_APP_LIB_PATH),$(patsubst %,$(dir)/%/src/$(BUILD_CORE),$(APP_LIBS_LIST)))

BUILD_APP_LIB_CPP_SRC = $(foreach dir,$(nrf5boards1100),$(wildcard $(dir)/*.cpp)) # */
BUILD_APP_LIB_C_SRC   = $(foreach dir,$(nrf5boards1100),$(wildcard $(dir)/*.c)) # */
BUILD_APP_LIB_H_SRC   = $(foreach dir,$(nrf5boards1100),$(wildcard $(dir)/*.h)) # */
BUILD_APP_LIB_H_SRC   = $(foreach dir,$(nrf5boards1100),$(wildcard $(dir)/*.hpp)) # */

BUILD_APP_LIB_OBJS     = $(patsubst $(APPLICATION_PATH)/%.cpp,$(OBJDIR)/%.cpp.o,$(BUILD_APP_LIB_CPP_SRC))
BUILD_APP_LIB_OBJS    += $(patsubst $(APPLICATION_PATH)/%.c,$(OBJDIR)/%.c.o,$(BUILD_APP_LIB_C_SRC))

APP_LIBS_LOCK = 1

# Sketchbook/Libraries path
# wildcard required for ~ management
# ?ibraries required for libraries and Libraries
#
ifeq ($(ARDUINO_180_LIBRARY_PATH)/preferences.txt,)
    $(error Error: run Arduino or panStamp once and define the sketchbook path)
endif

ifeq ($(shell if [ -d '$(SKETCHBOOK_DIR)' ]; then echo 1 ; fi ),)
    SKETCHBOOK_DIR = $(shell grep sketchbook.path $(ARDUINO_180_LIBRARY_PATH)/preferences.txt | cut -d = -f 2)
endif

ifeq ($(shell if [ -d '$(SKETCHBOOK_DIR)' ]; then echo 1 ; fi ),)
    $(error Error: sketchbook path not found)
endif

USER_LIB_PATH   = $(wildcard $(SKETCHBOOK_DIR)/?ibraries)

VARIANT         = $(call PARSE_BOARD,$(BOARD_TAG),build.variant)
VARIANT_PATH    = $(HARDWARE_PATH)/variants/$(VARIANT)

BUILD_SD_NAME    = $(call SEARCH_FOR,$(BOARD_TAGS_LIST),build.sd_name)
BUILD_SD_VERSION = $(call SEARCH_FOR,$(BOARD_TAGS_LIST),build.sd_version)
BUILD_SD_FLAGS   = $(call SEARCH_FOR,$(BOARD_TAGS_LIST),build.sd_flags)
BUILD_SD_DWID    = $(call SEARCH_FOR,$(BOARD_TAGS_LIST),build.sd_fwid)

# .ld script
#
LDSCRIPT_PATH    = $(VARIANT_PATH)
nrf5boards1200   = $(call SEARCH_FOR,$(BOARD_TAGS_LIST),build.ldscript)
nrf5boards1200a  = $(call SEARCH_FOR,$(BOARD_TAGS_LIST),build.chip)
nrf5boards1200b  = $(filter armgcc_%{build.chip}.ld,$(nrf5boards1200))

ifneq ($(nrf5boards1200b),)
    nrf5boards1200c         = $(shell echo $(nrf5boards1200b) | sed 's:{build.chip}:$(nrf5boards1200a):g') # | sed 's:.ld::')
    LDSCRIPT        = $(nrf5boards1200c)

else
    LDSCRIPT        = $(word $(words $(nrf5boards1200)),$(nrf5boards1200))
endif

VARIANT_CPP_SRCS    = $(wildcard $(VARIANT_PATH)/*.cpp) # */
VARIANT_OBJ_FILES   = $(VARIANT_CPP_SRCS:.cpp=.cpp.o)
VARIANT_OBJS        = $(patsubst $(HARDWARE_PATH)/%,$(OBJDIR)/%,$(VARIANT_OBJ_FILES))

ifeq ($(MAX_RAM_SIZE),)
    MAX_RAM_SIZE    = $(call SEARCH_FOR,$(BOARD_TAGS_LIST),upload.maximum_data_size)
endif
nrf5boards1600  = $(call SEARCH_FOR,$(BOARD_TAGS_LIST),upload.maximum_size)
MAX_FLASH_SIZE  = $(word $(words $(nrf5boards1600)),$(nrf5boards1600))

#MAX_FLASH_SIZE  = $(call SEARCH_FOR,$(BOARD_TAGS_LIST),upload.maximum_size)

# Tool-chain names
#
CC      = $(APP_TOOLS_PATH)/arm-none-eabi-gcc
CXX     = $(APP_TOOLS_PATH)/arm-none-eabi-g++
AR      = $(APP_TOOLS_PATH)/arm-none-eabi-ar
OBJDUMP = $(APP_TOOLS_PATH)/arm-none-eabi-objdump
OBJCOPY = $(APP_TOOLS_PATH)/arm-none-eabi-objcopy
SIZE    = $(APP_TOOLS_PATH)/arm-none-eabi-size
NM      = $(APP_TOOLS_PATH)/arm-none-eabi-nm

MCU_FLAG_NAME    = mcpu
MCU              = $(call PARSE_BOARD,$(BOARD_TAG),build.mcu)
F_CPU            = $(call PARSE_BOARD,$(BOARD_TAG),build.f_cpu)


# BBC micro:bit USB PID VID
#
USB_VID     := $(call PARSE_BOARD,$(BOARD_TAG),build.vid)
USB_PID     := $(call PARSE_BOARD,$(BOARD_TAG),build.pid)
USB_PRODUCT := $(call PARSE_BOARD,$(BOARD_TAG),build.usb_product)
USB_VENDOR  := $(call PARSE_BOARD,$(BOARD_TAG),build.usb_manufacturer)

ifneq ($(USB_VID),)
    USB_FLAGS    = -DUSB_VID=$(USB_VID)
    USB_FLAGS   += -DUSB_PID=$(USB_PID)
    USB_FLAGS   += -DUSBCON
    USB_FLAGS   += -DUSB_MANUFACTURER='$(USB_VENDOR)'
    USB_FLAGS   += -DUSB_PRODUCT='$(USB_PRODUCT)'
endif

# These includes should come first
# ever changing include paths and libraries
nrf5boards1400a  = $(call PARSE_FILE,compiler.nrf,flags,$(HARDWARE_PATH)/platform.txt)
nrf5boards1400b  = $(filter-out -DNRF5, $(nrf5boards1400a))
# Use of : instead of / for sed
nrf5boards1400c  = $(shell echo $(nrf5boards1400b) | sed 's:-I{nrf.sdk.path}:$(HARDWARE_PATH)/cores/nRF5/SDK:g')

SOFT_DEVICE     := $(call PARSE_BOARD,$(BOARD_TAG1),softdevice)
nrf5boards1400d  = $(shell echo $(nrf5boards1400c) | sed 's:{softdevice}:$(SOFT_DEVICE):g')

INCLUDE_PATH     = $(nrf5boards1400d)
# Too many folders
#INCLUDE_PATH      = $(shell find $(CORE_LIB_PATH) -type d)

# WAS: nffs.path={runtime.platform.path}/libraries/nffs/src
# NOW: nordic.path={build.core.path}/nordic
# WAS:
#nrf5boards1500a         = $(call PARSE_FILE,nffs,includes,$(HARDWARE_PATH)/platform.txt)
#INCLUDE_PATH    += $(shell echo $(nrf5boards1500a) | sed 's:-I{nffs.path}:$(HARDWARE_PATH)/libraries/nffs/src:g')

# Now the rest
INCLUDE_PATH    += $(CORE_LIB_PATH) $(APP_LIB_PATH) $(VARIANT_PATH) $(HARDWARE_PATH)
INCLUDE_PATH    += $(sort $(dir $(APP_LIB_CPP_SRC) $(APP_LIB_C_SRC) $(APP_LIB_H_SRC)))
INCLUDE_PATH    += $(sort $(dir $(BUILD_APP_LIB_CPP_SRC) $(BUILD_APP_LIB_C_SRC) $(BUILD_APP_LIB_H_SRC)))
INCLUDE_PATH    += $(OBJDIR)

# And even empty folders from the specific libraries
INCLUDE_PATH    += $(nrf5boards1000b) $(nrf5boards1000c)

ifneq ($(BOARD_TAG1),)
    D_FLAGS          = $(call PARSE_BOARD,$(BOARD_TAG1),build.extra_flags)
else
    D_FLAGS          = $(call PARSE_BOARD,$(BOARD_TAG),build.extra_flags)
endif
D_FLAGS         += $(call PARSE_BOARD,$(BOARD_TAG),build.lfclk_flags)
D_FLAGS         += -DNRF5 -DARDUINO_BBC_MICROBIT
D_FLAGS         += -DARDUINO_ARCH_NRF5
FIRST_O_IN_A     = $$(find $(BUILDS_PATH) -name gcc_startup_nrf52.S.o)


# Flags for gcc, g++ and linker
# ----------------------------------
#
# Common CPPFLAGS for gcc, g++, assembler and linker
#
CPPFLAGS     = $(OPTIMISATION) $(WARNING_FLAGS)
CPPFLAGS    += -$(MCU_FLAG_NAME)=$(MCU) -DF_CPU=$(F_CPU)
CPPFLAGS    += -u _printf_float
CPPFLAGS    += -ffunction-sections -fdata-sections -nostdlib -mthumb
CPPFLAGS    += --param max-inline-insns-single=500 -MMD
CPPFLAGS    += $(call PARSE_BOARD,$(BOARD_TAG),build.float_flags)
CPPFLAGS    += $(addprefix -D, $(PLATFORM_TAG)) $(D_FLAGS) $(BUILD_SD_FLAGS)
CPPFLAGS    += $(addprefix -I, $(INCLUDE_PATH))
# Specific CFLAGS for gcc only
# gcc uses CPPFLAGS and CFLAGS
#
CFLAGS       = -std=gnu11 -DSOFTDEVICE_PRESENT

# Specific CXXFLAGS for g++ only
# g++ uses CPPFLAGS and CXXFLAGS
#
CXXFLAGS     = -std=gnu++11 -fno-threadsafe-statics -fno-rtti -fno-exceptions

# Specific ASFLAGS for gcc assembler only
# gcc assembler uses CPPFLAGS and ASFLAGS
#
ASFLAGS      = -x assembler-with-cpp

LDFLAGS      = $(OPTIMISATION) $(WARNING_FLAGS) -Wl,--gc-sections -save-temps
LDFLAGS     += -$(MCU_FLAG_NAME)=$(MCU) --specs=nano.specs --specs=nosys.specs
LDFLAGS     += $(call PARSE_BOARD,$(BOARD_TAG),build.float_flags)
LDFLAGS     += -L$(CORE_LIB_PATH)/SDK/components/toolchain/gcc/
LDFLAGS     += -L$(CORE_LIB_PATH)/SDK/components/softdevice/$(SOFT_DEVICE)/toolchain/armgcc/
LDFLAGS     += -L$(VARIANT_PATH)
LDFLAGS     += -L$(HARDWARE_PATH)/cores/nRF5/linker
LDFLAGS     += -T $(LDSCRIPT) -mthumb
LDFLAGS     += -Wl,--cref -Wl,-Map,$(BUILDS_PATH)/embeddedcomputing.map # Output a cross reference table.
LDFLAGS     += -Wl,--check-sections -Wl,--gc-sections
LDFLAGS     += -Wl,--unresolved-symbols=report-all
LDFLAGS     += -Wl,--warn-common -Wl,--warn-section-align
LDFLAGS     += -u _printf_float

# Specific OBJCOPYFLAGS for objcopy only
# objcopy uses OBJCOPYFLAGS only
#
OBJCOPYFLAGS  = -v -Obinary

# Target
#
# J-Link requires HEX and no USB reset at 1200
#ifeq ($(UPLOADER),jlink)
TARGET_HEXBIN = $(TARGET_HEX)
#else
#    TARGET_HEXBIN = $(TARGET_BIN)

# Serial 1200 reset
#
#    USB_TOUCH := $(call PARSE_BOARD,$(BOARD_TAG),upload.use_1200bps_touch)
#    ifeq ($(USB_TOUCH),true)
#        USB_RESET  = python $(UTILITIES_PATH)/reset_1200.py
#    endif
#endif


# Commands
# ----------------------------------
# Link command
#
COMMAND_LINK    = $(CC) -L$(OBJDIR) $(LDFLAGS) $(OUT_PREPOSITION)$@ -L$(OBJDIR) $(LOCAL_OBJS) -Wl,--start-group -lm $(TARGET_A) -Wl,--end-group

# Copy command
COMMAND_COPY    = $(OBJCOPY) -O ihex $< $@


# Upload command
# Already defined earlier
#
#COMMAND_UPLOAD  = $(AVRDUDE_EXEC) $(AVRDUDE_COM_OPTS) $(AVRDUDE_OPTS) -P$(USED_SERIAL_PORT) -Uflash:w:$(TARGET_HEX):i

endif # NRF5_BOARDS_BOARDS

endif # MAKEFILE_NAME
