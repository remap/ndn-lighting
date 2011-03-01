################################################################################
# Automatically-generated file. Do not edit!
################################################################################

CC = /home/nesl/overo-oe/tmp/sysroots/i686-linux/usr/armv7a/bin/arm-angstrom-linux-gnueabi-gcc
CXX = /home/nesl/overo-oe/tmp/sysroots/i686-linux/usr/armv7a/bin/arm-angstrom-linux-gnueabi-g++
CPP = /home/neslexport/overo-oe/tmp/sysroots/i686-linux/usr/armv7a/bin/arm-angstrom-linux-gnueabi-cpp
AR = /home/neslexport/overo-oe/tmp/sysroots/i686-linux/usr/armv7a/bin/arm-angstrom-linux-gnueabi-ar
LD = /home/nesl/overo-oe/tmp/sysroots/i686-linux/usr/armv7a/bin/arm-angstrom-linux-gnueabi-ld
CFLAGS =  -g -O2
CXXFLAGS =  -g -O2
CPPFLAGS = -I//overo-oe/tmp/sysroots/armv7a-angstrom-linux-gnueabi/usr/include/ -I//home/nesl/Documents/ccn_cross-compile_work_101510/ccnx/csrc/include/
LDFLAGS = -L/overo-oe/tmp/sysroots/armv7a-angstrom-linux-gnueabi/usr/lib/ -L/overo-oe/tmp/sysroots/armv7a-angstrom-linux-gnueabi/lib/ -L/home/nesl/Documents/ccn_cross-compile_work_101510/ccnx/csrc/lib -lssl -lcrypto

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include lib/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: lightInterest

# Tool invocations
lightInterest: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MacOS X C Linker'
	$(CC) -o "lightInterest" $(OBJS) $(USER_OBJS) $(CPPFLAGS) $(LDFLAGS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) lightInterest
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
