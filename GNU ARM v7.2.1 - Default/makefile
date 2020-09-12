################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include usound/subdir.mk
-include spiflash/subdir.mk
-include protocol/bluetooth/lib/EFR32MG22/GCC/subdir.mk
-include platform/service/sleeptimer/src/subdir.mk
-include platform/emlib/src/subdir.mk
-include platform/emdrv/sleep/src/subdir.mk
-include platform/emdrv/nvm3/src/subdir.mk
-include platform/emdrv/dmadrv/src/subdir.mk
-include platform/Device/SiliconLabs/EFR32MG22/Source/GCC/subdir.mk
-include platform/Device/SiliconLabs/EFR32MG22/Source/subdir.mk
-include hardware/kit/common/drivers/subdir.mk
-include hardware/kit/common/bsp/subdir.mk
-include crypto_and_time/subdir.mk
-include app/bluetooth/common/util/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: bt-usonic.axf

# Tool invocations
bt-usonic.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GNU ARM C Linker'
	arm-none-eabi-gcc -g -gdwarf-2 -mcpu=cortex-m33 -mthumb -T "/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/efr32mg22c224f512im40.ld" -Xlinker --gc-sections -Xlinker -Map="bt-usonic.map" -mfpu=fpv5-sp-d16 -mfloat-abi=hard --specs=nano.specs -u _printf_float -o bt-usonic.axf -Wl,--start-group "./usound/mic_v2.o" "./usound/simple_dsp.o" "./usound/spkr.o" "./spiflash/bootloader-callback-stubs.o" "./spiflash/bootloader-callbacks.o" "./spiflash/btl_driver_delay.o" "./spiflash/btl_driver_spi.o" "./spiflash/btl_driver_util.o" "./spiflash/btl_storage_spiflash.o" "./platform/service/sleeptimer/src/sl_sleeptimer.o" "./platform/service/sleeptimer/src/sl_sleeptimer_hal_prortc.o" "./platform/service/sleeptimer/src/sl_sleeptimer_hal_rtcc.o" "./platform/emlib/src/em_assert.o" "./platform/emlib/src/em_burtc.o" "./platform/emlib/src/em_cmu.o" "./platform/emlib/src/em_core.o" "./platform/emlib/src/em_cryotimer.o" "./platform/emlib/src/em_crypto.o" "./platform/emlib/src/em_emu.o" "./platform/emlib/src/em_eusart.o" "./platform/emlib/src/em_gpio.o" "./platform/emlib/src/em_i2c.o" "./platform/emlib/src/em_ldma.o" "./platform/emlib/src/em_msc.o" "./platform/emlib/src/em_pdm.o" "./platform/emlib/src/em_prs.o" "./platform/emlib/src/em_rmu.o" "./platform/emlib/src/em_rtcc.o" "./platform/emlib/src/em_se.o" "./platform/emlib/src/em_system.o" "./platform/emlib/src/em_timer.o" "./platform/emlib/src/em_usart.o" "./platform/emdrv/sleep/src/sleep.o" "./platform/emdrv/nvm3/src/nvm3_default.o" "./platform/emdrv/nvm3/src/nvm3_hal_flash.o" "./platform/emdrv/nvm3/src/nvm3_lock.o" "./platform/emdrv/dmadrv/src/dmadrv.o" "./platform/Device/SiliconLabs/EFR32MG22/Source/GCC/startup_efr32mg22.o" "./platform/Device/SiliconLabs/EFR32MG22/Source/system_efr32mg22.o" "./hardware/kit/common/drivers/i2cspm.o" "./hardware/kit/common/drivers/mx25flash_spi.o" "./hardware/kit/common/drivers/retargetio.o" "./hardware/kit/common/drivers/retargetserial.o" "./hardware/kit/common/bsp/bsp_stk.o" "./app/bluetooth/common/util/infrastructure.o" "./app.o" "./application_properties.o" "./gatt_db.o" "./init_app.o" "./init_board.o" "./init_mcu.o" "./main.o" "./pti.o" "./spp_client_main.o" "./spp_utils.o" "/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/protocol/bluetooth/lib/EFR32MG22/GCC/libmbedtls.a" "/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/protocol/bluetooth/lib/EFR32MG22/GCC/binapploader.o" "/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/radio/rail_lib/autogen/librail_release/librail_efr32xg22_gcc_release.a" "/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/protocol/bluetooth/lib/EFR32MG22/GCC/libbluetooth.a" "/Users/nams/SimplicityStudio/v4_workspace/bt-usonic/platform/emdrv/nvm3/lib/libnvm3_CM33_gcc.a" -lm "./battery.o" "./bt_enconter.o" "./ota.o" "./storage.o" "./crypto_and_time/crypto.o" "./crypto_and_time/monocypher.o" "./crypto_and_time/timing.o" "./crypto_and_time/x25519-cortex-m4-gcc.o" "./platform/emlib/src/em_iadc.o" "./usound/alarm.o" -Wl,--end-group -Wl,--start-group -lgcc -lc -lnosys -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

	@echo 'Building hex file: bt-usonic.hex'
	arm-none-eabi-objcopy -O ihex "bt-usonic.axf" "bt-usonic.hex"
	@echo ' '

	@echo 'Building bin file: bt-usonic.bin'
	arm-none-eabi-objcopy -O binary "bt-usonic.axf" "bt-usonic.bin"
	@echo ' '

	@echo 'Building s37 file: bt-usonic.s37'
	arm-none-eabi-objcopy -O srec "bt-usonic.axf" "bt-usonic.s37"
	@echo ' '

	@echo 'Running size tool'
	arm-none-eabi-size "bt-usonic.axf" -A
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) bt-usonic.axf
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
