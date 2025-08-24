@echo off
echo Building Win32 API standalone version...

if not exist "build" mkdir build
cd build

cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release

echo.
echo Build completed. Executable is in build/Release/
pause