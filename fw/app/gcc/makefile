#------------------------------------------------------------------------------
#  Eddystone for trackr device 
#------------------------------------------------------------------------------

TARGET_BOARD   := trackr

BUZZER_SUPPORT := "yes"
DBGLOG_SUPPORT := "no"

ifeq ($(DBGLOG_SUPPORT), "yes") 
ifeq ($(BUZZER_SUPPORT), "yes")
$(error DBGLOG and BUZZER are mutually exclusive options)
endif
endif

#------------------------------------------------------------------------------
# Define relative paths to SDK components
#------------------------------------------------------------------------------

SDK_BASE      = ../../../..
COMPONENTS    = $(SDK_BASE)/components
TEMPLATE_PATH = $(COMPONENTS)/toolchain/gcc

OUTPUT_NAME   = trackr

#------------------------------------------------------------------------------
# Proceed cautiously beyond this point.  Little should change.
#------------------------------------------------------------------------------

export OUTPUT_NAME
export GNU_INSTALL_ROOT

MAKEFILE_NAME := $(MAKEFILE_LIST)
MAKEFILE_DIR := $(dir $(MAKEFILE_NAME) ) 

LINKER_SCRIPT := ./gcc_nrf51_s110_xxab.ld

ifeq ($(OS),Windows_NT)
  include $(TEMPLATE_PATH)/Makefile.windows
else
  include $(TEMPLATE_PATH)/Makefile.posix
endif

# echo suspend
ifeq ("$(VERBOSE)","1")
  NO_ECHO :=
else
  NO_ECHO := @
endif

# Toolchain commands
CC       := "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-gcc"
AS       := "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-as"
AR       := "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-ar" -r
LD       := "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-ld"
NM       := "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-nm"
OBJDUMP  := "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-objdump"
OBJCOPY  := "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-objcopy"
SIZE     := "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-size"
MK       := mkdir
RM       := rm -rf
CP       := cp

# function for removing duplicates in a list
remduplicates = $(strip $(if $1,$(firstword $1) $(call remduplicates,$(filter-out $(firstword $1),$1))))

# source common to all targets

C_SOURCE_FILES += ../main.c
C_SOURCE_FILES += ../advert.c
C_SOURCE_FILES += ../connect.c
C_SOURCE_FILES += ../ble_eddy.c
C_SOURCE_FILES += ../eddystone.c
C_SOURCE_FILES += ../battery.c
C_SOURCE_FILES += ../temperature.c
C_SOURCE_FILES += ../trackr_bsp.c
C_SOURCE_FILES += ../printf.c

ifeq ($(DBGLOG_SUPPORT), "yes")
	CFLAGS += -D DBGLOG_SUPPORT=1
	C_SOURCE_FILES += ../uart.c
	C_SOURCE_FILES += ../dump.c
endif

ifeq ($(BUZZER_SUPPORT), "yes")
	CFLAGS += -D BUZZER_SUPPORT=1
	C_SOURCE_FILES += ../buzzer.c
	C_SOURCE_FILES += ../tones.c
endif

C_SOURCE_FILES += $(COMPONENTS)/libraries/button/app_button.c
C_SOURCE_FILES += $(COMPONENTS)/libraries/fifo/app_fifo.c
C_SOURCE_FILES += $(COMPONENTS)/libraries/timer/app_timer.c
C_SOURCE_FILES += $(COMPONENTS)/libraries/scheduler/app_scheduler.c
C_SOURCE_FILES += $(COMPONENTS)/drivers_nrf/pstorage/pstorage.c
C_SOURCE_FILES += $(COMPONENTS)/libraries/util/nrf_assert.c
C_SOURCE_FILES += $(COMPONENTS)/drivers_nrf/hal/nrf_delay.c
C_SOURCE_FILES += $(COMPONENTS)/ble/common/ble_advdata.c
C_SOURCE_FILES += $(COMPONENTS)/ble/common/ble_conn_params.c
C_SOURCE_FILES += $(COMPONENTS)/ble/common/ble_srv_common.c
C_SOURCE_FILES += $(COMPONENTS)/ble/ble_radio_notification/ble_radio_notification.c
C_SOURCE_FILES += $(COMPONENTS)/toolchain/system_nrf51.c
C_SOURCE_FILES += $(COMPONENTS)/softdevice/common/softdevice_handler/softdevice_handler.c
C_SOURCE_FILES += $(COMPONENTS)/ble/device_manager/device_manager_peripheral.c
C_SOURCE_FILES += $(COMPONENTS)/libraries/gpiote/app_gpiote.c

# assembly files common to all targets
ASM_SOURCE_FILES += $(COMPONENTS)/toolchain/gcc/gcc_startup_nrf51.s

# includes common to all targets
INC_PATHS += -I../
INC_PATHS += -I$(COMPONENTS)/drivers_nrf/config
INC_PATHS += -I$(COMPONENTS)/libraries/util
INC_PATHS += -I$(COMPONENTS)/toolchain/gcc
INC_PATHS += -I$(COMPONENTS)/ble/common
INC_PATHS += -I$(COMPONENTS)/ble/ble_radio_notification
INC_PATHS += -I$(COMPONENTS)/drivers_nrf/common
INC_PATHS += -I$(COMPONENTS)/drivers_nrf/pstorage
INC_PATHS += -I$(COMPONENTS)/drivers_nrf/pstorage/config
INC_PATHS += -I$(COMPONENTS)/softdevice/s110/headers
INC_PATHS += -I$(COMPONENTS)/libraries/fifo
INC_PATHS += -I$(COMPONENTS)/drivers_nrf/hal
INC_PATHS += -I$(COMPONENTS)/drivers_nrf/pstorage
INC_PATHS += -I$(COMPONENTS)/libraries/trace
INC_PATHS += -I$(COMPONENTS)/libraries/gpiote
INC_PATHS += -I$(COMPONENTS)/toolchain
INC_PATHS += -I$(COMPONENTS)/device
INC_PATHS += -I$(COMPONENTS)/softdevice/common/softdevice_handler
INC_PATHS += -I$(COMPONENTS)/libraries/timer
INC_PATHS += -I$(COMPONENTS)/libraries/app_timer_appsh.h
INC_PATHS += -I$(COMPONENTS)/libraries/scheduler
INC_PATHS += -I$(COMPONENTS)/drivers_nrf/hal
INC_PATHS += -I$(COMPONENTS)/libraries/button
INC_PATHS += -I$(COMPONENTS)/ble/device_manager
INC_PATHS += -I$(COMPONENTS)/ble/device_manager/config

OBJECT_DIRECTORY = _build
LISTING_DIRECTORY = $(OBJECT_DIRECTORY)
OUTPUT_BINARY_DIRECTORY = $(OBJECT_DIRECTORY)

ifeq ($(MAKECMDGOALS),debug)
  BUILD_TYPE := debug
  DEBUG_FLAGS += -D DEBUG -g -O0
  CFLAGS += -Wa,-adhln
else
  BUILD_TYPE := release
  DEBUG_FLAGS += -D NDEBUG -O3
  CFLAGS += -Wa,-adhln
endif

# Sorting removes duplicates
BUILD_DIRECTORIES := $(sort $(OBJECT_DIRECTORY) $(OUTPUT_BINARY_DIRECTORY) $(LISTING_DIRECTORY) )

# flags common to all targets
#CFLAGS += -save-temps
CFLAGS += $(DEBUG_FLAGS)
CFLAGS += -D NRF51
CFLAGS += -D BLE_STACK_SUPPORT_REQD
CFLAGS += -D S110
CFLAGS += -D SOFTDEVICE_PRESENT
CFLAGS += -D $(TARGET_BOARD)
CFLAGS += -D BLE_DFU_APP_SUPPORT
CFLAGS += -mcpu=cortex-m0
CFLAGS += -mthumb -mabi=aapcs --std=gnu99
CFLAGS += -Wall -Werror
CFLAGS += -mfloat-abi=soft
CFLAGS += -ffunction-sections 
CFLAGS += -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin

LDFLAGS += -Xlinker -Map=$(LISTING_DIRECTORY)/$(OUTPUT_NAME).map
LDFLAGS += -mthumb -mabi=aapcs -L $(TEMPLATE_PATH) -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m0
LDFLAGS += $(DEBUG_FLAGS)
LDFLAGS += -Wl,--gc-sections
LDFLAGS += --specs=nano.specs -lc -lnosys

ASMFLAGS += $(DEBUG_FLAGS)
ASMFLAGS += -x assembler-with-cpp
ASMFLAGS += -D NRF51
ASMFLAGS += -D BLE_STACK_SUPPORT_REQD
ASMFLAGS += -D S110
ASMFLAGS += -D SOFTDEVICE_PRESENT
ASMFLAGS += -D $(TARGET_BOARD)

C_SOURCE_FILE_NAMES = $(notdir $(C_SOURCE_FILES))
C_PATHS = $(call remduplicates, $(dir $(C_SOURCE_FILES) ) )
C_OBJECTS = $(addprefix $(OBJECT_DIRECTORY)/, $(C_SOURCE_FILE_NAMES:.c=.o) )

ASM_SOURCE_FILE_NAMES = $(notdir $(ASM_SOURCE_FILES))
ASM_PATHS = $(call remduplicates, $(dir $(ASM_SOURCE_FILES) ))
ASM_OBJECTS = $(addprefix $(OBJECT_DIRECTORY)/, $(ASM_SOURCE_FILE_NAMES:.s=.o) )

TOOLCHAIN_BASE = $(basename $(notdir $(GNU_INSTALL_ROOT)))

vpath %.c $(C_PATHS)
vpath %.s $(ASM_PATHS)

OBJECTS  = $(C_OBJECTS) $(ASM_OBJECTS)
LISTINGS = $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).lst

all: $(BUILD_DIRECTORIES) $(OBJECTS)
	@echo Linking target: $(OUTPUT_NAME).elf
	$(NO_ECHO)$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf
	$(NO_ECHO)$(MAKE) -f $(MAKEFILE_NAME) -C $(MAKEFILE_DIR) -e finalize

	@echo "*****************************************************"
	@echo "build project: $(OUTPUT_NAME)"
	@echo "build type:    $(BUILD_TYPE)"
	@echo "build with:    $(TOOLCHAIN_BASE)"
	@echo "build target:  $(TARGET_BOARD)"
	@echo "build options  --"
	@echo "               BUZZER_SUPPORT     $(BUZZER_SUPPORT)"
	@echo "               DBGLOG_SUPPORT     $(DBGLOG_SUPPORT)"
	@echo "build products --"
	@echo "               $(OUTPUT_NAME).elf"
	@echo "               $(OUTPUT_NAME).hex"
	@echo "*****************************************************"

debug : all

release : all

# Create build directories
$(BUILD_DIRECTORIES):
	echo $(MAKEFILE_NAME)
	$(MK) $@

# Create objects from C SRC files
$(OBJECT_DIRECTORY)/%.o: %.c
	@echo Compiling module: $(notdir $<)
	$(NO_ECHO)$(CC) $(CFLAGS) $(INC_PATHS) \
	-c $< -o $@ > $(OUTPUT_BINARY_DIRECTORY)/$*.lst

# Assemble files
$(OBJECT_DIRECTORY)/%.o: %.s
	@echo Compiling module: $(notdir $<)
	$(NO_ECHO)$(CC) $(ASMFLAGS) $(INC_PATHS) -c -o $@ $<

# Link
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf: $(BUILD_DIRECTORIES) $(OBJECTS)
	@echo Linking target: $(OUTPUT_NAME).elf
	@echo LINKER_SCRIPT: $(LINKER_SCRIPT)
	$(NO_ECHO)$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf

# Create binary .bin file from the .elf file
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).bin: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf
	@echo Preparing: $(OUTPUT_NAME).bin
	$(NO_ECHO)$(OBJCOPY) -O binary $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).bin

# Create binary .hex file from the .elf file
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).hex: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf
	@echo Preparing: $(OUTPUT_NAME).hex
	$(NO_ECHO)$(OBJCOPY) -O ihex $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).hex

# Create disassemble .lst from the .elf file
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).lst: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf
	@echo Disasm listing: $(OUTPUT_NAME).lst
	$(NO_ECHO)$(OBJDUMP) -dSt $^ > $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).lst

finalize: $(LISTINGS) echosize

echosize:
	-@echo ""
	$(NO_ECHO)$(SIZE) $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf
	-@echo ""

clean:
	$(RM) $(BUILD_DIRECTORIES)


