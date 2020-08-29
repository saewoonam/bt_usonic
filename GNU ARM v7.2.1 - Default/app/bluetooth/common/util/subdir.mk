################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../app/bluetooth/common/util/infrastructure.c 

OBJS += \
./app/bluetooth/common/util/infrastructure.o 

C_DEPS += \
./app/bluetooth/common/util/infrastructure.d 


# Each subdirectory must supply rules for building sources it contributes
app/bluetooth/common/util/infrastructure.o: ../app/bluetooth/common/util/infrastructure.c
	@echo 'Building file: $<'
	@echo 'Invoking: GNU ARM C Compiler'
	arm-none-eabi-gcc -g -gdwarf-2 -mcpu=cortex-m33 -mthumb -std=c99 '-D__StackLimit=0x20000000' '-DHAL_CONFIG=1' '-DNVM3_DEFAULT_NVM_SIZE=24576' '-D__HEAP_SIZE=0xD00' '-D__STACK_SIZE=0x800' '-DEFR32MG22C224F512IM40=1' -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/Device/SiliconLabs/EFR32MG22/Include" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/CMSIS/Include" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/Device/SiliconLabs/EFR32MG22/Source" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/service/sleeptimer/config" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/emdrv/nvm3/inc" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/protocol/bluetooth/ble_stack/inc/common" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/hardware/kit/common/bsp" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/halconfig/inc/hal-config" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/emlib/inc" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/emlib/src" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/hardware/kit/common/halconfig" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/service/sleeptimer/src" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/Device/SiliconLabs/EFR32MG22/Source/GCC" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/hardware/kit/common/drivers" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/radio/rail_lib/common" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/emdrv/common/inc" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/radio/rail_lib/protocol/ieee802154" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/emdrv/sleep/inc" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/service/sleeptimer/inc" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/radio/rail_lib/protocol/ble" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/radio/rail_lib/chip/efr32/efr32xg2x" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/app/bluetooth/common/util" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/emdrv/nvm3/src" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/protocol/bluetooth/ble_stack/inc/soc" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/common/inc" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/bootloader/api" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/emdrv/sleep/src" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/emdrv/uartdrv/inc" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/emdrv/gpiointerrupt/inc" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/bootloader" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/emdrv/dmadrv/inc" -I"/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/hardware/kit/EFR32BG22_BRD4184A/config" -O3 -Wall -c -fmessage-length=0 -ffunction-sections -fdata-sections -mfpu=fpv5-sp-d16 -mfloat-abi=hard -MMD -MP -MF"app/bluetooth/common/util/infrastructure.d" -MT"app/bluetooth/common/util/infrastructure.o" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


