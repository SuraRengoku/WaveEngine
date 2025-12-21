@echo off
REM copy Vulkan Runtime DLL to output folder

set OUTPUT_DIR=%~1
set VULKAN_SDK=%VULKAN_SDK%

if "%OUTPUT_DIR%"=="" (
    echo Error: Output directory not specified
    exit /b 1
)

if "%VULKAN_SDK%"=="" (
    echo Warning: VULKAN_SDK environment variable not set, using default path
    set VULKAN_SDK=C:\VulkanSDK\1.3.283.0
)

echo Copying Vulkan Runtime to %OUTPUT_DIR%...

REM copy Vulkan Loader DLL
copy /Y "%VULKAN_SDK%\Bin\vulkan-1.dll" "%OUTPUT_DIR%\"

REM in Debug mode we still need to copy Validation Layer
if exist "%VULKAN_SDK%\Bin\VkLayer_khronos_validation.dll" (
    copy /Y "%VULKAN_SDK%\Bin\VkLayer_khronos_validation.dll" "%OUTPUT_DIR%\"
    copy /Y "%VULKAN_SDK%\Bin\VkLayer_khronos_validation.json" "%OUTPUT_DIR%\"
)

echo Done!