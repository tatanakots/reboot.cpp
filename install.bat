@echo off
setlocal

:: installer: attempts to copy platform-appropriate binaries into %WINDIR%\System32 and SysWOW64
:: Expected filenames in the same folder as this script:
:: - reboot-x64.exe (64-bit build)
:: - reboot-x86.exe (32-bit build)

set SCRIPT_DIR=%~dp0

:: Check for admin privileges
net session >nul 2>&1
if %errorlevel% NEQ 0 (
  echo Requesting elevation...
  powershell -Command "Start-Process -FilePath '%COMSPEC%' -ArgumentList '/c','\"%~f0\"' -Verb RunAs"
  exit /b
)

:: Determine architecture of the OS (not the process)
if defined PROCESSOR_ARCHITEW6432 (
  set ARCH=64
) else (
  if "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
    set ARCH=64
  ) else (
    set ARCH=32
  )
)

set WIN_DIR=%WINDIR%

echo Detected system architecture: %ARCH%-bit

if "%ARCH%"=="64" (
  if exist "%SCRIPT_DIR%reboot-x64.exe" (
    copy /Y "%SCRIPT_DIR%reboot-x64.exe" "%WIN_DIR%\System32\reboot.exe" >nul
    if %errorlevel% EQU 0 (
      echo Installed 64-bit binary to %WIN_DIR%\System32\reboot.exe
    ) else (
      echo Failed to copy 64-bit binary to %WIN_DIR%\System32
    )
  ) else (
    echo Warning: 64-bit binary not found at "%SCRIPT_DIR%reboot-x64.exe"
  )

  if exist "%SCRIPT_DIR%reboot-x86.exe" (
    copy /Y "%SCRIPT_DIR%reboot-x86.exe" "%WIN_DIR%\SysWOW64\reboot.exe" >nul
    if %errorlevel% EQU 0 (
      echo Installed 32-bit binary to %WIN_DIR%\SysWOW64\reboot.exe
    ) else (
      echo Failed to copy 32-bit binary to %WIN_DIR%\SysWOW64
    )
  ) else (
    echo Warning: 32-bit binary not found at "%SCRIPT_DIR%reboot-x86.exe"
  )
) else (
  if exist "%SCRIPT_DIR%reboot-x86.exe" (
    copy /Y "%SCRIPT_DIR%reboot-x86.exe" "%WIN_DIR%\System32\reboot.exe" >nul
    if %errorlevel% EQU 0 (
      echo Installed 32-bit binary to %WIN_DIR%\System32\reboot.exe
    ) else (
      echo Failed to copy 32-bit binary to %WIN_DIR%\System32
    )
  ) else (
    echo Warning: 32-bit binary not found at "%SCRIPT_DIR%reboot-x86.exe"
  )
)

echo Installation finished.
endlocal
pause
