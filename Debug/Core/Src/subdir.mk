################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/can.c \
../Core/Src/can1_can2_bridge_test.c \
../Core/Src/can2_demo.c \
../Core/Src/can2_loopback_test.c \
../Core/Src/can2_test.c \
../Core/Src/can_app.c \
../Core/Src/can_bus_diagnosis.c \
../Core/Src/can_diagnosis_test.c \
../Core/Src/can_dual_node.c \
../Core/Src/can_loop_test.c \
../Core/Src/can_periodic_send.c \
../Core/Src/can_simple_demo.c \
../Core/Src/can_trigger_send.c \
../Core/Src/freertos.c \
../Core/Src/main.c \
../Core/Src/mcp2515.c \
../Core/Src/mcp2515_test_demo.c \
../Core/Src/stm32f4xx_hal_msp.c \
../Core/Src/stm32f4xx_hal_timebase_tim.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f4xx.c \
../Core/Src/usart.c 

OBJS += \
./Core/Src/can.o \
./Core/Src/can1_can2_bridge_test.o \
./Core/Src/can2_demo.o \
./Core/Src/can2_loopback_test.o \
./Core/Src/can2_test.o \
./Core/Src/can_app.o \
./Core/Src/can_bus_diagnosis.o \
./Core/Src/can_diagnosis_test.o \
./Core/Src/can_dual_node.o \
./Core/Src/can_loop_test.o \
./Core/Src/can_periodic_send.o \
./Core/Src/can_simple_demo.o \
./Core/Src/can_trigger_send.o \
./Core/Src/freertos.o \
./Core/Src/main.o \
./Core/Src/mcp2515.o \
./Core/Src/mcp2515_test_demo.o \
./Core/Src/stm32f4xx_hal_msp.o \
./Core/Src/stm32f4xx_hal_timebase_tim.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f4xx.o \
./Core/Src/usart.o 

C_DEPS += \
./Core/Src/can.d \
./Core/Src/can1_can2_bridge_test.d \
./Core/Src/can2_demo.d \
./Core/Src/can2_loopback_test.d \
./Core/Src/can2_test.d \
./Core/Src/can_app.d \
./Core/Src/can_bus_diagnosis.d \
./Core/Src/can_diagnosis_test.d \
./Core/Src/can_dual_node.d \
./Core/Src/can_loop_test.d \
./Core/Src/can_periodic_send.d \
./Core/Src/can_simple_demo.d \
./Core/Src/can_trigger_send.d \
./Core/Src/freertos.d \
./Core/Src/main.d \
./Core/Src/mcp2515.d \
./Core/Src/mcp2515_test_demo.d \
./Core/Src/stm32f4xx_hal_msp.d \
./Core/Src/stm32f4xx_hal_timebase_tim.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f4xx.d \
./Core/Src/usart.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/can.cyclo ./Core/Src/can.d ./Core/Src/can.o ./Core/Src/can.su ./Core/Src/can1_can2_bridge_test.cyclo ./Core/Src/can1_can2_bridge_test.d ./Core/Src/can1_can2_bridge_test.o ./Core/Src/can1_can2_bridge_test.su ./Core/Src/can2_demo.cyclo ./Core/Src/can2_demo.d ./Core/Src/can2_demo.o ./Core/Src/can2_demo.su ./Core/Src/can2_loopback_test.cyclo ./Core/Src/can2_loopback_test.d ./Core/Src/can2_loopback_test.o ./Core/Src/can2_loopback_test.su ./Core/Src/can2_test.cyclo ./Core/Src/can2_test.d ./Core/Src/can2_test.o ./Core/Src/can2_test.su ./Core/Src/can_app.cyclo ./Core/Src/can_app.d ./Core/Src/can_app.o ./Core/Src/can_app.su ./Core/Src/can_bus_diagnosis.cyclo ./Core/Src/can_bus_diagnosis.d ./Core/Src/can_bus_diagnosis.o ./Core/Src/can_bus_diagnosis.su ./Core/Src/can_diagnosis_test.cyclo ./Core/Src/can_diagnosis_test.d ./Core/Src/can_diagnosis_test.o ./Core/Src/can_diagnosis_test.su ./Core/Src/can_dual_node.cyclo ./Core/Src/can_dual_node.d ./Core/Src/can_dual_node.o ./Core/Src/can_dual_node.su ./Core/Src/can_loop_test.cyclo ./Core/Src/can_loop_test.d ./Core/Src/can_loop_test.o ./Core/Src/can_loop_test.su ./Core/Src/can_periodic_send.cyclo ./Core/Src/can_periodic_send.d ./Core/Src/can_periodic_send.o ./Core/Src/can_periodic_send.su ./Core/Src/can_simple_demo.cyclo ./Core/Src/can_simple_demo.d ./Core/Src/can_simple_demo.o ./Core/Src/can_simple_demo.su ./Core/Src/can_trigger_send.cyclo ./Core/Src/can_trigger_send.d ./Core/Src/can_trigger_send.o ./Core/Src/can_trigger_send.su ./Core/Src/freertos.cyclo ./Core/Src/freertos.d ./Core/Src/freertos.o ./Core/Src/freertos.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/mcp2515.cyclo ./Core/Src/mcp2515.d ./Core/Src/mcp2515.o ./Core/Src/mcp2515.su ./Core/Src/mcp2515_test_demo.cyclo ./Core/Src/mcp2515_test_demo.d ./Core/Src/mcp2515_test_demo.o ./Core/Src/mcp2515_test_demo.su ./Core/Src/stm32f4xx_hal_msp.cyclo ./Core/Src/stm32f4xx_hal_msp.d ./Core/Src/stm32f4xx_hal_msp.o ./Core/Src/stm32f4xx_hal_msp.su ./Core/Src/stm32f4xx_hal_timebase_tim.cyclo ./Core/Src/stm32f4xx_hal_timebase_tim.d ./Core/Src/stm32f4xx_hal_timebase_tim.o ./Core/Src/stm32f4xx_hal_timebase_tim.su ./Core/Src/stm32f4xx_it.cyclo ./Core/Src/stm32f4xx_it.d ./Core/Src/stm32f4xx_it.o ./Core/Src/stm32f4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f4xx.cyclo ./Core/Src/system_stm32f4xx.d ./Core/Src/system_stm32f4xx.o ./Core/Src/system_stm32f4xx.su ./Core/Src/usart.cyclo ./Core/Src/usart.d ./Core/Src/usart.o ./Core/Src/usart.su

.PHONY: clean-Core-2f-Src

