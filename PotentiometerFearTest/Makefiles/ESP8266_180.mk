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
# Last update: 12 May 2020 release 11.7.17
#


ifeq ($(MAKEFILE_NAME),)

ESP8266_1 = $(ARDUINO_180_PACKAGES_PATH)/esp8266

ifneq ($(wildcard $(ESP8266_1)),)
    ESP8266_APP     = $(ESP8266_1)
    ESP8266_PATH    = $(ESP8266_APP)
    ESP8266_BOARDS  = $(ESP8266_1)/hardware/esp8266/$(ESP8266_RELEASE)/boards.txt
endif

ifneq ($(call PARSE_FILE,$(BOARD_TAG),name,$(ESP8266_BOARDS)),)
MAKEFILE_NAME = ESP8266_180


# ESP8266 specifics
# ----------------------------------
#
PLATFORM         := esp8266
PLATFORM_TAG      = ARDUINO=10812 ARDUINO_ARCH_ESP8266 EMBEDXCODE=$(RELEASE_NOW) ARDUINO_$(BUILD_BOARD) ESP8266 ARDUINO_BOARD=\"$(BUILD_BOARD)\"
APPLICATION_PATH := $(ESP8266_PATH)
PLATFORM_VERSION := $(ESP8266_RELEASE) for Arduino $(ARDUINO_IDE_RELEASE)

HARDWARE_PATH     = $(APPLICATION_PATH)/hardware/esp8266/$(ESP8266_RELEASE)
TOOL_CHAIN_PATH   = $(APPLICATION_PATH)/tools/xtensa-lx106-elf-gcc/$(ESP8266_EXTENSA_RELEASE)
OTHER_TOOLS_PATH  = $(APPLICATION_PATH)/tools

BOARDS_TXT      := $(HARDWARE_PATH)/boards.txt
BUILD_CORE       = $(call PARSE_BOARD,$(BOARD_TAG),build.core)
SUB_PLATFORM     = $(BUILD_CORE)
BUILD_BOARD      = $(call PARSE_BOARD,$(BOARD_TAG),build.board)

PYTHON_EXEC      = /usr/bin/python
#PYTHON_EXEC      = $(OTHER_TOOLS_PATH)/python3/$(ESP8266_PYTHON_RELEASE)/python3

#ESP_POST_COMPILE   = $(APPLICATION_PATH)/tools/esptool/$(ESP8266_TOOLS_RELEASE)/esptool
ESP_POST_COMPILE   = $(PYTHON_EXEC) $(HARDWARE_PATH)/tools/elf2bin.py
BOOTLOADER_ELF     = $(HARDWARE_PATH)/bootloaders/eboot/eboot.elf

ESP_PRE_COMPILE    = $(PYTHON_EXEC) $(HARDWARE_PATH)/tools/signing.py


# Release check
# ----------------------------------
#
REQUIRED_ESP8266_RELEASE = 2.7.1
ifeq ($(shell if [[ '$(ESP8266_RELEASE)' > '$(REQUIRED_ESP8266_RELEASE)' ]] || [[ '$(ESP8266_RELEASE)' = '$(REQUIRED_ESP8266_RELEASE)' ]]; then echo 1 ; else echo 0 ; fi ),0)
$(error ESP8266 release $(REQUIRED_ESP8266_RELEASE) or later required, release $(ESP8266_RELEASE) installed)
endif

# Complicated menu system for Arduino 1.5
# Another example of Arduino's quick and dirty job
#
BOARD_TAGS_LIST   = $(BOARD_TAG) $(BOARD_TAG1) $(BOARD_TAG2) $(BOARD_TAG3) $(BOARD_TAG4) $(BOARD_TAG5) $(BOARD_TAG6) $(BOARD_TAG7)

SEARCH_FOR  = $(strip $(foreach t,$(1),$(call PARSE_BOARD,$(t),$(2))))

# flash_size is defined twice for nodemcu and nodemcuv2, take first
#
BUILD_FLASH_SIZE   = $(firstword $(call SEARCH_FOR,$(BOARD_TAGS_LIST),build.flash_size))
BUILD_FLASH_FREQ   = $(firstword $(call SEARCH_FOR,$(BOARD_TAGS_LIST),build.flash_freq))

# ESP8266 changes the uploader again!
#
UPLOADER            = esptool
    UPLOADER_PATH       = $(HARDWARE_PATH)/tools
    UPLOADER_EXEC       = $(PYTHON_EXEC) $(UPLOADER_PATH)/upload.py
    UPLOADER_OPTS       = --chip esp8266 --port $(USED_SERIAL_PORT) --baud $(call SEARCH_FOR,$(BOARD_TAGS_LIST),upload.speed) $(call SEARCH_FOR,$(BOARD_TAGS_LIST),upload.erase_cmd) $(call SEARCH_FOR,$(BOARD_TAGS_LIST),upload.resetmethod)

APP_TOOLS_PATH      := $(TOOL_CHAIN_PATH)/bin
CORE_LIB_PATH       := $(HARDWARE_PATH)/cores/esp8266



# Take assembler file as first
#
APP_LIB_PATH        := $(HARDWARE_PATH)/libraries
CORE_AS_SRCS         = $(wildcard $(CORE_LIB_PATH)/*.S) # */
esp001               = $(patsubst %.S,%.S.o,$(filter %S, $(CORE_AS_SRCS)))
FIRST_O_IN_A         = $(patsubst $(APPLICATION_PATH)/%,$(OBJDIR)/%,$(esp001))
#FIRST_O_IN_A     = $(filter %/$(esp001),$(BUILD_CORE_OBJS))

# Endless options for ESP8266
#
INCLUDE_LWIPVARIANT = $(call SEARCH_FOR,$(BOARD_TAGS_LIST),build.lwip_include)
ifeq ($(INCLUDE_LWIPVARIANT),)
    INCLUDE_LWIPVARIANT = lwip/include
endif

CPPFLAGS_LWIPVARIANT = $(call SEARCH_FOR,$(BOARD_TAGS_LIST),build.lwip_flags)
ifeq ($(CPPFLAGS_LWIPVARIANT),)
    CPPFLAGS_LWIPVARIANT = -DLWIP_OPEN_SRC
endif

L_FLAGS_LWIPVARIANT = $(call SEARCH_FOR,$(BOARD_TAGS_LIST),build.lwip_lib)
ifeq ($(L_FLAGS_LWIPVARIANT),)
    L_FLAGS_LWIPVARIANT = -llwip_gcc
endif

MAKE_LWIPVARIANT = $(call SEARCH_FOR,$(BOARD_TAGS_LIST),recipe.hooks.sketch.prebuild.1.pattern)
ifneq ($(MAKE_LWIPVARIANT),)
    $(error Option 'v1.4 Compile from source' not supported)
#    MAKE_RESULT = $(shell make -C $(HARDWARE_PATH)/tools/sdk/lwip/src install TOOLS_PATH=$(APP_TOOLS_PATH)/xtensa-lx106-elf-)
endif

# IDE version management, based on the SDK version
#
#$(eval SDK_VERSION = $(shell cat $(UPLOADER_PATH)/sdk/version))
#ifeq ($(SDK_VERSION),1.0.0)
#    BOARD_TAG      := generic
#    L_FLAGS         = -lhal -lphy -lpp -lnet80211 -llwip_gcc -lwpa -lcrypto -lmain -lwps -lbearssl -laxtls -lespnow -lsmartconfig -lairkiss -lwpa2 -lstdc++ -lm -lc -lgcc
L_FLAGS         = -lhal -lphy -lpp -lnet80211 $(L_FLAGS_LWIPVARIANT) -lwpa -lcrypto -lmain -lwps -lbearssl -laxtls -lespnow -lsmartconfig -lairkiss -lwpa2 -lstdc++ -lm -lc -lgcc

    ADDRESS_BIN1     = 00000
#    ADDRESS_BIN2    = 40000
#else
# For ESP8266 1.6.1
#    L_FLAGS         = -lm -lc -lgcc -lhal -lphy -lnet80211 -llwip -lwpa -lmain -lpp -lsmartconfig
#    ADDRESS_BIN2    = 10000
#endif

# Sketchbook/Libraries path
# wildcard required for ~ management
# ?ibraries required for libraries and Libraries
#
ifeq ($(USER_LIBRARY_DIR)/Arduino15/preferences.txt,)
    $(error Error: run Arduino once and define the sketchbook path)
endif

ifeq ($(shell if [ -d '$(SKETCHBOOK_DIR)' ]; then echo 1 ; fi ),)
    SKETCHBOOK_DIR = $(shell grep sketchbook.path $(wildcard ~/Library/Arduino15/preferences.txt) | cut -d = -f 2)
endif

ifeq ($(shell if [ -d '$(SKETCHBOOK_DIR)' ]; then echo 1 ; fi ),)
    $(error Error: sketchbook path not found)
endif

USER_LIB_PATH  = $(wildcard $(SKETCHBOOK_DIR)/?ibraries)

VARIANT      = $(call PARSE_BOARD,$(BOARD_TAG),build.variant)
VARIANT_PATH = $(HARDWARE_PATH)/variants/$(VARIANT)

VARIANT_CPP_SRCS  = $(wildcard $(VARIANT_PATH)/*.cpp) # */
VARIANT_OBJ_FILES = $(VARIANT_CPP_SRCS:.cpp=.cpp.o)
VARIANT_OBJS      = $(patsubst $(VARIANT_PATH)/%,$(OBJDIR)/%,$(VARIANT_OBJ_FILES))

# Tool-chain names
#
CC      = $(APP_TOOLS_PATH)/xtensa-lx106-elf-gcc
CXX     = $(APP_TOOLS_PATH)/xtensa-lx106-elf-g++
AR      = $(APP_TOOLS_PATH)/xtensa-lx106-elf-ar
OBJDUMP = $(APP_TOOLS_PATH)/xtensa-lx106-elf-objdump
OBJCOPY = $(APP_TOOLS_PATH)/xtensa-lx106-elf-objcopy
SIZE    = $(APP_TOOLS_PATH)/xtensa-lx106-elf-size
NM      = $(APP_TOOLS_PATH)/xtensa-lx106-elf-nm

MCU_FLAG_NAME    = # mmcu
MCU              = $(call SEARCH_FOR,$(BOARD_TAGS_LIST),build.mcu)
F_CPU            = $(call SEARCH_FOR,$(BOARD_TAGS_LIST),build.f_cpu)

MAX_FLASH_SIZE   = $(firstword $(call SEARCH_FOR,$(BOARD_TAGS_LIST),upload.maximum_size))

OPTIMISATION     = -Os -g

INCLUDE_PATH     = $(HARDWARE_PATH)/tools/sdk/include
INCLUDE_PATH    += $(HARDWARE_PATH)/tools/sdk/$(INCLUDE_LWIPVARIANT)
INCLUDE_PATH    += $(HARDWARE_PATH)/tools/sdk/libc/xtensa-lx106-elf/include
INCLUDE_PATH    += $(CORE_LIB_PATH)
INCLUDE_PATH    += $(VARIANT_PATH)
INCLUDE_PATH    += $(BUILDS_PATH)

LDSCRIPT = $(call SEARCH_FOR,$(BOARD_TAGS_LIST),build.flash_ld)

D_FLAGS      = -D$(BUILD_SDK)=1
#D_FLAGS     += $(call SEARCH_FOR,$(BOARD_TAGS_LIST),build.lwip_flags)
D_FLAGS     += $(call SEARCH_FOR,$(BOARD_TAGS_LIST),build.flash_flags)

# For release 2.6.1
#BUILD_SDK = NONOSDK22x_191024
# For release 2.6.2, actually an option
BUILD_SDK = NONOSDK22x_190703

# Flags for gcc, g++ and linker
# ----------------------------------
#
# Common CPPFLAGS for gcc, g++, assembler and linker
#
CPPFLAGS     = -g $(OPTIMISATION) $(WARNING_FLAGS)
CPPFLAGS    += -D__ets__ -DICACHE_FLASH -U__STRICT_ANSI__ $(CPPFLAGS_LWIPVARIANT)
CPPFLAGS    += -mlongcalls -mtext-section-literals -falign-functions=4 -MMD
CPPFLAGS    += -ffunction-sections -fdata-sections
CPPFLAGS    += -DF_CPU=$(F_CPU)
CPPFLAGS    += $(addprefix -D, $(PLATFORM_TAG) $(BUILD_BOARD)) $(D_FLAGS)
CPPFLAGS    += $(addprefix -I, $(INCLUDE_PATH))

# Specific CFLAGS for gcc only
# gcc uses CPPFLAGS and CFLAGS
#
CFLAGS       = -Wpointer-arith -Wno-implicit-function-declaration -Wl,-EL -fno-inline-functions -nostdlib -std=gnu99
# was -std=c99

# Specific CXXFLAGS for g++ only
# g++ uses CPPFLAGS and CXXFLAGS
#
CXXFLAGS     = -fno-exceptions -fno-rtti -std=gnu++11

# Specific ASFLAGS for gcc assembler only
# gcc assembler uses CPPFLAGS and ASFLAGS
#
ASFLAGS      = -x assembler-with-cpp

# Specific LDFLAGS for linker only
# linker uses CPPFLAGS and LDFLAGS
#
LDFLAGS      = $(OPTIMISATION) $(WARNING_FLAGS)
#-Wl,--gc-sections
LDFLAGS     += -nostdlib -Wl,--no-check-sections
LDFLAGS     += -u app_entry -u _printf_float -u _scanf_float
LDFLAGS     += -fno-exceptions -Wl,-static
LDFLAGS     += -L$(BUILDS_PATH)
LDFLAGS     += -L$(HARDWARE_PATH)/tools/sdk/lib
LDFLAGS     += -L$(HARDWARE_PATH)/tools/sdk/lib/$(BUILD_SDK)
LDFLAGS     += -L$(HARDWARE_PATH)/tools/sdk/ld
LDFLAGS     += -L$(HARDWARE_PATH)/tools/sdk/libc/xtensa-lx106-elf/lib
LDFLAGS     += -T $(LDSCRIPT)
LDFLAGS     += -Wl,--gc-sections -Wl,-wrap,system_restart_local -Wl,-wrap,spi_flash_read


# Specific OBJCOPYFLAGS for objcopy only
# objcopy uses OBJCOPYFLAGS only
#
#OBJCOPYFLAGS  = $(call PARSE_BOARD,$(BOARD_TAG),build.flash_mode)
OBJCOPYFLAGS  = $(firstword $(call SEARCH_FOR,$(BOARD_TAGS_LIST),build.flash_mode))

# Target
#
TARGET_HEXBIN = $(TARGET_BIN2)


# Commands
# ----------------------------------
# Link command
#
EXTRA_COMMAND   = $(CC) -CC -E -P -DVTABLES_IN_FLASH $(HARDWARE_PATH)/tools/sdk/ld/eagle.app.v6.common.ld.h -o $(BUILDS_PATH)/local.eagle.app.v6.common.ld

COMMAND_LINK    = $(CC) $(LDFLAGS) $(OUT_PREPOSITION)$@ -Wl,--start-group $(LOCAL_OBJS) $(TARGET_A) $(L_FLAGS) -Wl,--end-group -L$(BUILDS_PATH)

PRE_COMPILE_COMMAND = $(ESP_PRE_COMPILE) --mode header --publickey  "{build.source.path}/public.key" --out "{build.path}/core/Updater_Signing.h"

POST_COMPILE_COMMAND = $(ESP_POST_COMPILE) --eboot $(BOOTLOADER_ELF) --app $(TARGET_ELF) --flash_mode $(OBJCOPYFLAGS) --flash_freq $(BUILD_FLASH_FREQ) --flash_size $(BUILD_FLASH_SIZE) --path $(APP_TOOLS_PATH) --out $(BUILDS_PATH)/$(TARGET_NAME)_$(ADDRESS_BIN1).bin

#    COMMAND_UPLOAD  = $(UPLOADER_EXEC) $(UPLOADER_OPTS) --trace version --end $(UPLOADER_OPTS) --trace write_flash 0x$(ADDRESS_BIN1) $(BUILDS_PATH)/$(TARGET_NAME)_$(ADDRESS_BIN1).bin --end
    COMMAND_UPLOAD  = $(UPLOADER_EXEC) $(UPLOADER_OPTS) write_flash 0x0 $(BUILDS_PATH)/$(TARGET_NAME)_$(ADDRESS_BIN1).bin

endif # ESP8266_BOARDS

endif # MAKEFILE_NAME
