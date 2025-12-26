# reboot.cpp

A tiny cross-platform command-line utility to immediately reboot, power off, or halt the local machine.

Origin
- This project was created because Windows does not include `reboot`, `poweroff`, or `halt` commands by default in PowerShell/CMD. This tool provides lightweight single-file binaries named `reboot`, `poweroff`, and `halt` (or `.exe` on Windows) for quick use from the shell, compatible with Linux halt(8) options.

What it does
- Supports reboot, poweroff, and halt operations with various options.
- On Windows it adjusts process token privileges and calls the OS shutdown/reboot APIs.
- On Linux it calls the `reboot(2)` syscall with appropriate flags (requires root).
- On macOS it sends Apple Events for restart or shutdown (requires root or appropriate privileges).

Security and privileges
- Rebooting a system requires administrator/root privileges. The tool will fail with an error if run without sufficient rights.
- Copying binaries into `%WINDIR%\System32` or `%WINDIR%\SysWOW64` requires elevation. The included `install.bat` will try to elevate itself when run.
- Always review binaries and scripts before running them with elevated rights.

Build / compile

Windows (MinGW or Visual Studio)

```powershell
# Using MinGW (cross-compile or on Windows host with MinGW):
x86_64-w64-mingw32-g++ -o reboot-x64.exe reboot.cpp -ladvapi32
i686-w64-mingw32-g++ -o reboot-x86.exe reboot.cpp -ladvapi32

# Then create hard links for poweroff and halt binaries
mklink /H poweroff-x64.exe reboot-x64.exe
mklink /H halt-x64.exe reboot-x64.exe
mklink /H poweroff-x86.exe reboot-x86.exe
mklink /H halt-x86.exe reboot-x86.exe
```

Linux

```bash
g++ -o reboot reboot.cpp
ln -s reboot poweroff
ln -s reboot halt
```

macOS (Apple Silicon or Intel)

```bash
clang++ -o reboot reboot.cpp -framework CoreFoundation -framework ApplicationServices
ln -s reboot poweroff
ln -s reboot halt
```

Repository artifacts produced by CI
- Windows artifact: `reboot-windows` contains `reboot-x64.exe`, `reboot-x86.exe`, and `install.bat` (one-click installer script).
- Linux artifact: `reboot-linux` (containing `reboot`).
- macOS artifact: `reboot-macos` (containing `reboot`).

Windows installer script
- `install.bat` tries to elevate (using PowerShell) if not run as Administrator.
- Installs `reboot.exe`, `poweroff.exe`, and `halt.exe` into `%WINDIR%\System32` and `%WINDIR%\SysWOW64` as appropriate.
- When elevated it detects host architecture and copies the appropriate binaries:
  - On 64-bit Windows: installs `reboot-x64.exe` -> `%WINDIR%\System32\reboot.exe` and `reboot-x86.exe` -> `%WINDIR%\SysWOW64\reboot.exe`.
  - On 32-bit Windows: installs `reboot-x86.exe` -> `%WINDIR%\System32\reboot.exe`.

CI / GitHub Actions
- The included workflow (`.github/workflows/build.yml`) builds for Windows (both x86 and x64 via MSYS2), Linux, and macOS, then uploads platform-specific artifacts.

Usage
- After building or downloading the right binary:
  - Linux/macOS: run `sudo ./reboot` (or just `./reboot` if you are root)
  - Windows: open an elevated CMD/PowerShell and run `reboot.exe` (or after installation simply `reboot` from any elevated shell)

Notes
- This is a low-level, immediate reboot tool. It does not attempt a graceful shutdown sequence beyond what the platform APIs provide when forcing a reboot.
- Use carefully on machines with unsaved work.
