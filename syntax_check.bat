@echo off
echo ========================================
echo CAN Bus Diagnosis Code Syntax Check
echo ========================================
echo.

echo [INFO] Checking syntax of newly added files...
echo.

REM Check if GCC is available for syntax checking
where gcc >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [WARNING] GCC not found in PATH
    echo [INFO] Cannot perform syntax check
    echo [INFO] Please compile using STM32CubeIDE to verify syntax
    goto :end
)

echo [CHECK] Checking can_bus_diagnosis.c...
gcc -c -I"Core/Inc" -I"Drivers/STM32F4xx_HAL_Driver/Inc" -I"Drivers/CMSIS/Device/ST/STM32F4xx/Include" -I"Drivers/CMSIS/Include" -I"Middlewares/Third_Party/FreeRTOS/Source/include" -I"Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2" -I"Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F" -DSTM32F407xx -fsyntax-only "Core/Src/can_bus_diagnosis.c" 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [PASS] can_bus_diagnosis.c syntax OK
) else (
    echo [FAIL] can_bus_diagnosis.c has syntax errors
)

echo [CHECK] Checking can_diagnosis_test.c...
gcc -c -I"Core/Inc" -I"Drivers/STM32F4xx_HAL_Driver/Inc" -I"Drivers/CMSIS/Device/ST/STM32F4xx/Include" -I"Drivers/CMSIS/Include" -I"Middlewares/Third_Party/FreeRTOS/Source/include" -I"Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2" -I"Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F" -DSTM32F407xx -fsyntax-only "Core/Src/can_diagnosis_test.c" 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [PASS] can_diagnosis_test.c syntax OK
) else (
    echo [FAIL] can_diagnosis_test.c has syntax errors
)

echo.
echo [INFO] Syntax check completed
echo [NOTE] For full compilation, use STM32CubeIDE

:end
echo ========================================
pause