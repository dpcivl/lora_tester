################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/logger/src/logger.c \
../Core/Src/logger/src/logger_platform.c 

OBJS += \
./Core/Src/logger/src/logger.o \
./Core/Src/logger/src/logger_platform.o 

C_DEPS += \
./Core/Src/logger/src/logger.d \
./Core/Src/logger/src/logger_platform.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/logger/src/%.o Core/Src/logger/src/%.su Core/Src/logger/src/%.cyclo: ../Core/Src/logger/src/%.c Core/Src/logger/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F746xx -c -I../Core/Inc -I../FATFS/Target -I../FATFS/App -I../USB_HOST/App -I../USB_HOST/Target -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1 -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/CDC/Inc -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/wogud/Documents/hyoin/workspace/tdd-practice/lora_tester/lora_tester_stm32/Core/Src/logger" -I"C:/Users/wogud/Documents/hyoin/workspace/tdd-practice/lora_tester/lora_tester_stm32/Core/Src/logger/src" -I"C:/Users/wogud/Documents/hyoin/workspace/tdd-practice/lora_tester/lora_tester_stm32/Core/Src/uart" -I"C:/Users/wogud/Documents/hyoin/workspace/tdd-practice/lora_tester/lora_tester_stm32/Core/Src/uart/inc" -I"C:/Users/wogud/Documents/hyoin/workspace/tdd-practice/lora_tester/lora_tester_stm32/Core/Src/logger/inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-logger-2f-src

clean-Core-2f-Src-2f-logger-2f-src:
	-$(RM) ./Core/Src/logger/src/logger.cyclo ./Core/Src/logger/src/logger.d ./Core/Src/logger/src/logger.o ./Core/Src/logger/src/logger.su ./Core/Src/logger/src/logger_platform.cyclo ./Core/Src/logger/src/logger_platform.d ./Core/Src/logger/src/logger_platform.o ./Core/Src/logger/src/logger_platform.su

.PHONY: clean-Core-2f-Src-2f-logger-2f-src

