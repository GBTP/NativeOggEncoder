@echo off
setlocal

set PROJECT_DIR=%~dp0..
set BUILD_DIR=%PROJECT_DIR%\build\windows-x64
set OUTPUT_DIR=%PROJECT_DIR%\..\AnoawaWorkspace\Anoawa\Assets\Plugins\NativeOggEncoder\x64-windows

cmake -S "%PROJECT_DIR%" -B "%BUILD_DIR%" -A x64 ^
    -DNOE_BUILD_SHARED=ON ^
    -DNOE_BUILD_STATIC=OFF ^
    -DCMAKE_BUILD_TYPE=Release

if %errorlevel% neq 0 exit /b %errorlevel%

cmake --build "%BUILD_DIR%" --config Release --parallel

if %errorlevel% neq 0 exit /b %errorlevel%

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
copy /Y "%BUILD_DIR%\Release\native_ogg_encoder.dll" "%OUTPUT_DIR%\"
echo Done: %OUTPUT_DIR%\native_ogg_encoder.dll
