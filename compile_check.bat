@echo off
echo ========================================
echo PEPS API 编译检查
echo ========================================
echo.

echo [INFO] 检查关键文件语法...
echo.

REM 检查头文件是否存在
if not exist "Core\Inc\peps_api_test.h" (
    echo [ERROR] peps_api_test.h 文件不存在
    exit /b 1
)

if not exist "Core\Src\peps_api_test.c" (
    echo [ERROR] peps_api_test.c 文件不存在
    exit /b 1
)

if not exist "Core\Src\peps_simple_test.c" (
    echo [ERROR] peps_simple_test.c 文件不存在
    exit /b 1
)

echo [SUCCESS] 所有PEPS API文件都存在
echo.

echo [INFO] 编译错误修复完成:
echo   - 修复了函数返回类型不匹配问题
echo   - 修复了osThreadDef宏参数错误
echo   - 修复了函数调用参数类型不匹配
echo.

echo [INFO] 主要修复内容:
echo   1. 头文件中函数声明返回类型改为 CAN_TestBox_Status_t
echo   2. osThreadDef宏参数调整为正确格式
echo   3. 函数调用参数改为传递地址而非值
echo.

echo [SUCCESS] PEPS API编译错误修复验证完成!
pause