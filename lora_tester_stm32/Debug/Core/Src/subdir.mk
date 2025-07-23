################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/CommandSender.c \
../Core/Src/LoraStarter.c \
../Core/Src/Network.c \
../Core/Src/ResponseHandler.c \
../Core/Src/SDStorage.c \
../Core/Src/freertos.c \
../Core/Src/main.c \
../Core/Src/stm32f7xx_hal_msp.c \
../Core/Src/stm32f7xx_hal_timebase_tim.c \
../Core/Src/stm32f7xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f7xx.c \
../Core/Src/time_common.c \
../Core/Src/time_stm32.c 

OBJS += \
./Core/Src/CommandSender.o \
./Core/Src/LoraStarter.o \
./Core/Src/Network.o \
./Core/Src/ResponseHandler.o \
./Core/Src/SDStorage.o \
./Core/Src/freertos.o \
./Core/Src/main.o \
./Core/Src/stm32f7xx_hal_msp.o \
./Core/Src/stm32f7xx_hal_timebase_tim.o \
./Core/Src/stm32f7xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f7xx.o \
./Core/Src/time_common.o \
./Core/Src/time_stm32.o 

C_DEPS += \
./Core/Src/CommandSender.d \
./Core/Src/LoraStarter.d \
./Core/Src/Network.d \
./Core/Src/ResponseHandler.d \
./Core/Src/SDStorage.d \
./Core/Src/freertos.d \
./Core/Src/main.d \
./Core/Src/stm32f7xx_hal_msp.d \
./Core/Src/stm32f7xx_hal_timebase_tim.d \
./Core/Src/stm32f7xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f7xx.d \
./Core/Src/time_common.d \
./Core/Src/time_stm32.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F746xx -c -I../Core/Inc -I../FATFS/Target -I../FATFS/App -I../USB_HOST/App -I../USB_HOST/Target -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1 -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/CDC/Inc -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/wogud/Documents/hyoin/workspace/tdd-practice/lora_tester/lora_tester_stm32/Core/Src/logger" -I"C:/Users/wogud/Documents/hyoin/workspace/tdd-practice/lora_tester/lora_tester_stm32/Core/Src/logger/src" -I"C:/Users/wogud/Documents/hyoin/workspace/tdd-practice/lora_tester/lora_tester_stm32/Core/Src/uart" -I"C:/Users/wogud/Documents/hyoin/workspace/tdd-practice/lora_tester/lora_tester_stm32/Core/Src/uart/inc" -I"C:/Users/wogud/Documents/hyoin/workspace/tdd-practice/lora_tester/lora_tester_stm32/Core/Src/logger/inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/CommandSender.cyclo ./Core/Src/CommandSender.d ./Core/Src/CommandSender.o ./Core/Src/CommandSender.su ./Core/Src/LoraStarter.cyclo ./Core/Src/LoraStarter.d ./Core/Src/LoraStarter.o ./Core/Src/LoraStarter.su ./Core/Src/Network.cyclo ./Core/Src/Network.d ./Core/Src/Network.o ./Core/Src/Network.su ./Core/Src/ResponseHandler.cyclo ./Core/Src/ResponseHandler.d ./Core/Src/ResponseHandler.o ./Core/Src/ResponseHandler.su ./Core/Src/SDStorage.cyclo ./Core/Src/SDStorage.d ./Core/Src/SDStorage.o ./Core/Src/SDStorage.su ./Core/Src/freertos.cyclo ./Core/Src/freertos.d ./Core/Src/freertos.o ./Core/Src/freertos.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/stm32f7xx_hal_msp.cyclo ./Core/Src/stm32f7xx_hal_msp.d ./Core/Src/stm32f7xx_hal_msp.o ./Core/Src/stm32f7xx_hal_msp.su ./Core/Src/stm32f7xx_hal_timebase_tim.cyclo ./Core/Src/stm32f7xx_hal_timebase_tim.d ./Core/Src/stm32f7xx_hal_timebase_tim.o ./Core/Src/stm32f7xx_hal_timebase_tim.su ./Core/Src/stm32f7xx_it.cyclo ./Core/Src/stm32f7xx_it.d ./Core/Src/stm32f7xx_it.o ./Core/Src/stm32f7xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f7xx.cyclo ./Core/Src/system_stm32f7xx.d ./Core/Src/system_stm32f7xx.o ./Core/Src/system_stm32f7xx.su ./Core/Src/time_common.cyclo ./Core/Src/time_common.d ./Core/Src/time_common.o ./Core/Src/time_common.su ./Core/Src/time_stm32.cyclo ./Core/Src/time_stm32.d ./Core/Src/time_stm32.o ./Core/Src/time_stm32.su

.PHONY: clean-Core-2f-Src

