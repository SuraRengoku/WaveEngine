@echo off
setlocal EnableDelayedExpansion
REM Try to set console to UTF-8 so IDE terminals (Rider/etc.) display non-ASCII output correctly.
REM Note: For this to work the file encoding and the terminal encoding must match (see notes below).
chcp 65001 >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    REM failed to switch to UTF-8; leave current code page
    REM On older systems chcp 65001 may not be supported. In that case the terminal may still show
    REM localized OS messages (e.g. 在 Windows 系统为中文) — if Rider shows garbage, see notes.
) else (
    REM switched to UTF-8 successfully
)

REM Copy Vulkan Runtime DLL to output folder

set "OUTPUT_DIR=%~1"

REM Remove trailing backslash if present to avoid quote escaping issues
if "%OUTPUT_DIR:~-1%"=="\" set "OUTPUT_DIR=%OUTPUT_DIR:~0,-1%"

if "%OUTPUT_DIR%"=="" (
    echo Error: Output directory not specified
    exit /b 1
)

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%" >nul 2>&1

echo Copying Vulkan Runtime to "%OUTPUT_DIR%"...

REM Try to find vulkan-1.dll in multiple locations
set VULKAN_DLL_FOUND=0

REM 1. Check VULKAN_SDK\Bin
if defined VULKAN_SDK (
    if exist "%VULKAN_SDK%\Bin\vulkan-1.dll" (
        echo Found vulkan-1.dll in VULKAN_SDK\Bin
        copy /Y "%VULKAN_SDK%\Bin\vulkan-1.dll" "%OUTPUT_DIR%\" >nul 2>&1
        set VULKAN_DLL_FOUND=1
    )
)

REM 2. Check VULKAN_SDK\Lib (some SDK versions)
if !VULKAN_DLL_FOUND! EQU 0 (
    if defined VULKAN_SDK (
        if exist "%VULKAN_SDK%\Lib\vulkan-1.dll" (
            echo Found vulkan-1.dll in VULKAN_SDK\Lib
            copy /Y "%VULKAN_SDK%\Lib\vulkan-1.dll" "%OUTPUT_DIR%\" >nul 2>&1
            set VULKAN_DLL_FOUND=1
        )
    )
)

REM 3. Check System32 (driver-provided)
if !VULKAN_DLL_FOUND! EQU 0 (
    if exist "C:\Windows\System32\vulkan-1.dll" (
        echo Found vulkan-1.dll in System32. Using system runtime.
        set VULKAN_DLL_FOUND=1
    )
)

REM 4. If still not found, skip copy (driver will provide at runtime)
if !VULKAN_DLL_FOUND! EQU 0 (
    echo Warning: vulkan-1.dll not found in SDK or System32
    echo The application will use the driver-provided Vulkan loader at runtime
    echo This is normal for modern Vulkan installations
)

REM Copy Validation Layers (Debug only)
if defined VULKAN_SDK (
    if exist "%VULKAN_SDK%\Bin\VkLayer_khronos_validation.dll" (
        echo Copying Validation Layers...
        copy /Y "%VULKAN_SDK%\Bin\VkLayer_khronos_validation.dll" "%OUTPUT_DIR%\" >nul 2>&1
        
        if exist "%VULKAN_SDK%\Bin\VkLayer_khronos_validation.json" (
            copy /Y "%VULKAN_SDK%\Bin\VkLayer_khronos_validation.json" "%OUTPUT_DIR%\" >nul 2>&1
        )
    )
)

echo Done! Vulkan Runtime setup completed.
exit /b 0