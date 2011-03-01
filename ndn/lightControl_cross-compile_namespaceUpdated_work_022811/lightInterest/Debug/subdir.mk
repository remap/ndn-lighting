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

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ndnInterst.c 

OBJS += \
./ndnInterst.o 

C_DEPS += \
./ndnInterst.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	$(CC) -Icypto -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<" $(CPPFLAGS) $(LDFLAGS)
	@echo 'Finished building: $<'
	@echo ' '


