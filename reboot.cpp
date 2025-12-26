// reboot.cpp
// Cross-platform immediate reboot/poweroff/halt using only native APIs
// No output on success. English messages only.

#include <iostream>
#include <cstring>
#include <string>

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

enum Action { ACTION_REBOOT, ACTION_HALT, ACTION_POWEROFF };

struct Options {
    Action action;
    bool force;
    bool wtmp_only;
    bool no_wtmp;
    bool no_wall;
};

static void print_help(const char* progname) {
    std::string action;
    if (std::strstr(progname, "reboot")) action = "Reboot";
    else if (std::strstr(progname, "poweroff")) action = "Power off";
    else if (std::strstr(progname, "halt")) action = "Halt";
    else action = "Reboot";

    std::cout << "Usage: " << progname << " [OPTIONS...]\n"
                 "\n"
                 "\033[1m" << action << " the system.\033[0m\n"
                 "\n"
                 "Options:\n"
                 "     --help      Show this help\n"
                 "     --halt      Halt the machine\n"
                 "  -p --poweroff  Switch off the machine\n"
                 "     --reboot    Reboot the machine\n"
                 "  -f --force     Force immediate halt/power-off/reboot\n"
                 "  -w --wtmp-only Don't halt/power-off/reboot, just write wtmp record\n"
                 "  -d --no-wtmp   Don't write wtmp record\n"
                 "     --no-wall   Don't send wall message before halt/power-off/reboot\n"
                 "\n"
                 "See the halt(8) man page for details.\n";
}

static bool parse_options(int argc, char* argv[], Options& opts, const char* progname) {
    // Set default action based on program name
    if (std::strstr(progname, "reboot")) opts.action = ACTION_REBOOT;
    else if (std::strstr(progname, "poweroff")) opts.action = ACTION_POWEROFF;
    else if (std::strstr(progname, "halt")) opts.action = ACTION_HALT;
    else opts.action = ACTION_REBOOT;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            print_help(progname);
            return false; // indicate to exit
        } else if (arg == "--halt") {
            opts.action = ACTION_HALT;
        } else if (arg == "-p" || arg == "--poweroff") {
            opts.action = ACTION_POWEROFF;
        } else if (arg == "--reboot") {
            opts.action = ACTION_REBOOT;
        } else if (arg == "-f" || arg == "--force") {
            opts.force = true;
        } else if (arg == "-w" || arg == "--wtmp-only") {
            opts.wtmp_only = true;
        } else if (arg == "-d" || arg == "--no-wtmp") {
            opts.no_wtmp = true;
        } else if (arg == "--no-wall") {
            opts.no_wall = true;
        } else {
            std::cerr << "Error: Unknown option '" << arg << "'. Use --help for help.\n";
            return false;
        }
    }
    return true;
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
    // Extract program name
    const char* progname = argv[0];
    const char* basename = std::strrchr(progname, '\\');
    if (!basename) basename = std::strrchr(progname, '/');
    if (basename) progname = basename + 1;

    Options opts = {ACTION_REBOOT, false, false, false, false};
    if (!parse_options(argc, argv, opts, progname)) {
        return 1; // error or help shown
    }

    // If wtmp_only, just simulate (do nothing for now, as wtmp is Linux-specific)
    if (opts.wtmp_only) {
        // On non-Linux, just exit
        return 0;
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

    // Determine shutdown type
    BOOL bRebootAfterShutdown = (opts.action == ACTION_REBOOT);
    DWORD dwShutdownFlags = SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG;

    BOOL ret = InitiateSystemShutdownEx(
        NULL, NULL, 0, TRUE, bRebootAfterShutdown, dwShutdownFlags
    );

    CloseHandle(hToken);

    if (!ret) {
        std::string action_str = (opts.action == ACTION_REBOOT) ? "reboot" :
                                 (opts.action == ACTION_POWEROFF) ? "poweroff" : "halt";
        std::cerr << "Error: Failed to initiate " << action_str << " (code: " << GetLastError() << ").\n";
        return 1;
    }

#else
    // Unix-like
    #ifdef __APPLE__
        if (geteuid() != 0) {
            std::cerr << "Error: Root privileges required.\n";
            return 1;
        }
        AEEventID eventID;
        if (opts.action == ACTION_REBOOT) {
            eventID = kAERestart;
        } else {
            eventID = kAEShutDown; // for halt and poweroff
        }
        OSStatus status = SendAppleEventToSystemProcess(eventID);
        if (status != noErr) {
            std::string action_str = (opts.action == ACTION_REBOOT) ? "restart" : "shutdown";
            std::cerr << "Error: Failed to send " << action_str << " event (code: " << status << "). Requires root or System Settings permission.\n";
            return 1;
        }
    #else
        // Linux: direct syscall
        if (geteuid() != 0) {
            std::cerr << "Error: Root privileges required.\n";
            return 1;
        }

        sync();  // Flush filesystems

        int reboot_cmd;
        if (opts.action == ACTION_REBOOT) {
            reboot_cmd = RB_AUTOBOOT;
        } else if (opts.action == ACTION_HALT) {
            reboot_cmd = RB_HALT_SYSTEM;
        } else { // ACTION_POWEROFF
            reboot_cmd = RB_POWER_OFF;
        }

        int ret = reboot(reboot_cmd);
        if (ret == -1) {
            std::string action_str = (opts.action == ACTION_REBOOT) ? "reboot" :
                                     (opts.action == ACTION_HALT) ? "halt" : "poweroff";
            std::cerr << "Error: " << action_str << "() syscall failed: " << strerror(errno) << "\n";
            return 1;
        }
    #endif
#endif

    // Success: no output
    return 0;
}