@echo off
REM pacakge release

set BUILD_CONFIG=Release
set OUTPUT_DIR=.\Build\Release

echo Building %BUILD_CONFIG% configuration...
msbuild WaveEngine.sln /p:Configuration=%BUILD_CONFIG% /p:Platform=x64

echo Packaging files...
mkdir "%OUTPUT_DIR%\Redist"

REM copy exe file
copy /Y "x64\%BUILD_CONFIG%\EngineTest.exe" "%OUTPUT_DIR%\"

REM copy Vulkan Runtime
copy /Y "%VULKAN_SDK%\Bin\vulkan-1.dll" "%OUTPUT_DIR%\"

REM copy other dependencies(if exist)
REM copy /Y "path\to\other.dll" "%OUTPUT_DIR%\"

echo Packaging complete! Output in %OUTPUT_DIR%
pause