// reboot.cpp
// Cross-platform immediate reboot using only native APIs
// No output on success. English messages only.

#include <iostream>
#include <cstring>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/reboot.h>
    #include <errno.h>
    #ifdef __linux__
        #include <linux/reboot.h>
    #endif
    #ifdef __APPLE__
        #include <CoreServices/CoreServices.h>
        #include <Carbon/Carbon.h>
    #endif
#endif

static void print_help() {
    std::cout << "Usage: reboot [-h|--help]\n"
                 "Immediately reboots the computer.\n"
                 "Requires administrator/root privileges.\n";
}

#ifdef __APPLE__
static OSStatus SendAppleEventToSystemProcess(AEEventID eventToSendID) {
    AEAddressDesc targetDesc;
    static const ProcessSerialNumber kPSNOfSystemProcess = {0, kSystemProcess};
    AppleEvent eventReply = {typeNull, NULL};
    AppleEvent eventToSend = {typeNull, NULL};

    OSStatus status = AECreateDesc(typeProcessSerialNumber, &kPSNOfSystemProcess,
                                   sizeof(kPSNOfSystemProcess), &targetDesc);
    if (status != noErr) return status;

    status = AECreateAppleEvent(kCoreEventClass, eventToSendID, &targetDesc,
                                kAutoGenerateReturnID, kAnyTransactionID, &eventToSend);
    AEDisposeDesc(&targetDesc);
    if (status != noErr) return status;

    status = AESendMessage(&eventToSend, &eventReply, kAENormalPriority, kAEDefaultTimeout);
    AEDisposeDesc(&eventToSend);
    if (status != noErr) return status;

    AEDisposeDesc(&eventReply);
    return status;
}
#endif

int main(int argc, char* argv[]) {
    if (argc > 1) {
        if (std::strcmp(argv[1], "-h") == 0 || std::strcmp(argv[1], "--help") == 0) {
            print_help();
            return 0;
        } else {
            std::cerr << "Error: Unknown argument. Use -h for help.\n";
            return 1;
        }
    }

#ifdef _WIN32
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp{};

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        std::cerr << "Error: Failed to open process token.\n";
        return 1;
    }

    if (!LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid)) {
        CloseHandle(hToken);
        std::cerr << "Error: LookupPrivilegeValue failed.\n";
        return 1;
    }

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0)) {
        CloseHandle(hToken);
        std::cerr << "Error: AdjustTokenPrivileges failed.\n";
        return 1;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
        CloseHandle(hToken);
        std::cerr << "Error: Insufficient privileges. Run as Administrator.\n";
        return 1;
    }

    // Force reboot, no timeout, close all apps
    BOOL ret = InitiateSystemShutdownEx(
        NULL, NULL, 0, TRUE, TRUE,
        SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG
    );

    CloseHandle(hToken);

    if (!ret) {
        std::cerr << "Error: Failed to initiate reboot (code: " << GetLastError() << ").\n";
        return 1;
    }

#else
    // Unix-like: use reboot(2) syscall with RB_AUTOBOOT
    #ifdef __APPLE__
        if (geteuid() != 0) {
            std::cerr << "Error: Root privileges required.\n";
            return 1;
        }
        OSStatus status = SendAppleEventToSystemProcess(kAERestart);
        if (status != noErr) {
            std::cerr << "Error: Failed to send restart event (code: " << status << "). Requires root or System Settings permission.\n";
            return 1;
        }
    #else
        // Linux: direct syscall
        if (geteuid() != 0) {
            std::cerr << "Error: Root privileges required.\n";
            return 1;
        }

        sync();  // Flush filesystems

        int ret = reboot(RB_AUTOBOOT);
        if (ret == -1) {
            std::cerr << "Error: reboot() syscall failed: " << strerror(errno) << "\n";
            return 1;
        }
    #endif
#endif

    // Success: no output
    return 0;
}