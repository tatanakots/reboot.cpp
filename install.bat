@echo off
setlocal

:: installer: attempts to copy platform-appropriate binaries into %WINDIR%\System32 and SysWOW64
:: Expected filenames in the same folder as this script:
:: - reboot-x64.exe (64-bit build)
:: - reboot-x86.exe (32-bit build)
:: Will create reboot.exe, and hard links for poweroff.exe, halt.exe

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
  set SRC_64=%SCRIPT_DIR%reboot-x64.exe
  set SRC_32=%SCRIPT_DIR%reboot-x86.exe
  set DST_DIR_64=%WIN_DIR%\System32
  set DST_DIR_32=%WIN_DIR%\SysWOW64
) else (
  set SRC_32=%SCRIPT_DIR%reboot-x86.exe
  set DST_DIR_64=%WIN_DIR%\System32
  set DST_DIR_32=%WIN_DIR%\System32
)

for %%c in (reboot poweroff halt) do (
  if "%ARCH%"=="64" (
    if exist "%SRC_64%" (
      if "%%c"=="reboot" (
        copy /Y "%SRC_64%" "%DST_DIR_64%\%%c.exe" >nul
        if %errorlevel% EQU 0 (
          echo Installed 64-bit %%c to %DST_DIR_64%\%%c.exe
        ) else (
          echo Failed to copy 64-bit %%c to %DST_DIR_64%
        )
      ) else (
        mklink /H "%DST_DIR_64%\%%c.exe" "%DST_DIR_64%\reboot.exe" >nul
        if %errorlevel% EQU 0 (
          echo Created hard link for 64-bit %%c to %DST_DIR_64%\%%c.exe
        ) else (
          echo Failed to create hard link for 64-bit %%c
        )
      )
    ) else (
      echo Warning: 64-bit binary not found at "%SRC_64%"
    )

    if exist "%SRC_32%" (
      if "%%c"=="reboot" (
        copy /Y "%SRC_32%" "%DST_DIR_32%\%%c.exe" >nul
        if %errorlevel% EQU 0 (
          echo Installed 32-bit %%c to %DST_DIR_32%\%%c.exe
        ) else (
          echo Failed to copy 32-bit %%c to %DST_DIR_32%
        )
      ) else (
        mklink /H "%DST_DIR_32%\%%c.exe" "%DST_DIR_32%\reboot.exe" >nul
        if %errorlevel% EQU 0 (
          echo Created hard link for 32-bit %%c to %DST_DIR_32%\%%c.exe
        ) else (
          echo Failed to create hard link for 32-bit %%c
        )
      )
    ) else (
      echo Warning: 32-bit binary not found at "%SRC_32%"
    )
  ) else (
    if exist "%SRC_32%" (
      if "%%c"=="reboot" (
        copy /Y "%SRC_32%" "%DST_DIR_64%\%%c.exe" >nul
        if %errorlevel% EQU 0 (
          echo Installed 32-bit %%c to %DST_DIR_64%\%%c.exe
        ) else (
          echo Failed to copy 32-bit %%c to %DST_DIR_64%
        )
      ) else (
        mklink /H "%DST_DIR_64%\%%c.exe" "%DST_DIR_64%\reboot.exe" >nul
        if %errorlevel% EQU 0 (
          echo Created hard link for 32-bit %%c to %DST_DIR_64%\%%c.exe
        ) else (
          echo Failed to create hard link for 32-bit %%c
        )
      )
    ) else (
      echo Warning: 32-bit binary not found at "%SRC_32%"
    )
  )
)

echo Installation finished.
endlocal
pause
