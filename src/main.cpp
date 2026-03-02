#include <windows.h>

#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
volatile BOOL g_shouldRun = TRUE;

struct Config {
    std::string port;
    DWORD baud = 115200;
    bool hex = true;
    bool ascii = false;
    bool stats = false;
};

void PrintUsage(const char* exe) {
    std::cout << "Usage:\n"
              << "  " << exe << " --port COM5 --baud 115200 [--hex] [--ascii] [--stats]\n\n"
              << "Options:\n"
              << "  --port <COMx>   Required. Serial port (example: COM5)\n"
              << "  --baud <rate>   Optional. Baud rate (default: 115200)\n"
              << "  --hex           Optional. Print hex dump (default: on)\n"
              << "  --ascii         Optional. Print printable ASCII alongside hex\n"
              << "  --stats         Optional. Print bytes/sec every 1 second\n"
              << "  --help          Show this help\n";
}

bool ParseUnsigned(const std::string& value, DWORD& out) {
    if (value.empty()) {
        return false;
    }

    char* end = nullptr;
    const unsigned long parsed = std::strtoul(value.c_str(), &end, 10);
    if (end == value.c_str() || *end != '\0' || parsed > 0xFFFFFFFFul) {
        return false;
    }

    out = static_cast<DWORD>(parsed);
    return true;
}

bool ParseArgs(int argc, char** argv, Config& cfg, std::string& err) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            PrintUsage(argv[0]);
            return false;
        }

        if (arg == "--port") {
            if (i + 1 >= argc) {
                err = "Missing value for --port";
                return false;
            }
            cfg.port = argv[++i];
            continue;
        }

        if (arg == "--baud") {
            if (i + 1 >= argc) {
                err = "Missing value for --baud";
                return false;
            }
            DWORD parsed = 0;
            if (!ParseUnsigned(argv[++i], parsed) || parsed == 0) {
                err = "Invalid baud rate: " + std::string(argv[i]);
                return false;
            }
            cfg.baud = parsed;
            continue;
        }

        if (arg == "--hex") {
            cfg.hex = true;
            continue;
        }

        if (arg == "--ascii") {
            cfg.ascii = true;
            continue;
        }

        if (arg == "--stats") {
            cfg.stats = true;
            continue;
        }

        err = "Unknown argument: " + arg;
        return false;
    }

    if (cfg.port.empty()) {
        err = "--port is required";
        return false;
    }

    return true;
}

std::string FormatWindowsError(DWORD error) {
    LPSTR buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD len = FormatMessageA(flags, nullptr, error, 0, reinterpret_cast<LPSTR>(&buffer), 0, nullptr);

    std::string msg;
    if (len > 0 && buffer != nullptr) {
        msg.assign(buffer, len);
        while (!msg.empty() && (msg.back() == '\r' || msg.back() == '\n' || msg.back() == ' ' || msg.back() == '\t')) {
            msg.pop_back();
        }
    } else {
        msg = "Unknown error";
    }

    if (buffer != nullptr) {
        LocalFree(buffer);
    }

    return msg;
}

std::string NormalizePortPath(const std::string& port) {
    if (port.rfind("\\\\.\\", 0) == 0) {
        return port;
    }
    return "\\\\.\\" + port;
}

std::string MakeAsciiPreview(const uint8_t* data, size_t size) {
    std::string out;
    out.reserve(size);
    for (size_t i = 0; i < size; ++i) {
        const unsigned char c = data[i];
        out.push_back(std::isprint(c) ? static_cast<char>(c) : '.');
    }
    return out;
}

void PrintRx(const uint8_t* data, DWORD size, bool showHex, bool showAscii) {
    std::ostringstream oss;
    oss << "[RX] " << size << " bytes";

    if (showHex) {
        oss << ": ";
        oss << std::hex << std::uppercase << std::setfill('0');
        for (DWORD i = 0; i < size; ++i) {
            if (i > 0) {
                oss << ' ';
            }
            oss << std::setw(2) << static_cast<unsigned int>(data[i]);
        }
    }

    if (showAscii) {
        oss << " | " << MakeAsciiPreview(data, size);
    }

    std::cout << oss.str() << "\n";
}

BOOL WINAPI ConsoleHandler(DWORD signal) {
    switch (signal) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            g_shouldRun = FALSE;
            return TRUE;
        default:
            return FALSE;
    }
}

}  // namespace

int main(int argc, char** argv) {
    Config cfg;
    std::string argError;
    if (!ParseArgs(argc, argv, cfg, argError)) {
        if (!argError.empty()) {
            std::cerr << "[ERROR] " << argError << "\n\n";
            PrintUsage(argv[0]);
            return 1;
        }
        return 0;
    }

    if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
        std::cerr << "[WARN] Failed to install Ctrl+C handler: "
                  << FormatWindowsError(GetLastError()) << "\n";
    }

    const std::string portPath = NormalizePortPath(cfg.port);
    HANDLE serial = CreateFileA(
        portPath.c_str(),
        GENERIC_READ,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (serial == INVALID_HANDLE_VALUE) {
        const DWORD err = GetLastError();
        std::cerr << "[ERROR] Failed to open " << cfg.port << " @ " << cfg.baud << ": "
                  << FormatWindowsError(err);
        if (err == ERROR_FILE_NOT_FOUND) {
            std::cerr << " (port not found)";
        } else if (err == ERROR_ACCESS_DENIED) {
            std::cerr << " (port is busy or access denied)";
        }
        std::cerr << "\n";
        return 1;
    }

    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(serial, &dcb)) {
        std::cerr << "[ERROR] GetCommState failed: " << FormatWindowsError(GetLastError()) << "\n";
        CloseHandle(serial);
        return 1;
    }

    dcb.BaudRate = cfg.baud;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;

    if (!SetCommState(serial, &dcb)) {
        std::cerr << "[ERROR] SetCommState failed: " << FormatWindowsError(GetLastError()) << "\n";
        CloseHandle(serial);
        return 1;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    if (!SetCommTimeouts(serial, &timeouts)) {
        std::cerr << "[ERROR] SetCommTimeouts failed: " << FormatWindowsError(GetLastError()) << "\n";
        CloseHandle(serial);
        return 1;
    }

    PurgeComm(serial, PURGE_RXCLEAR | PURGE_RXABORT);

    std::cout << "[OK] Opened " << cfg.port << " @ " << cfg.baud << "\n";

    std::vector<uint8_t> buffer(1024);
    std::uint64_t statsBytes = 0;
    auto lastStats = std::chrono::steady_clock::now();

    while (g_shouldRun) {
        DWORD bytesRead = 0;
        if (!ReadFile(serial, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr)) {
            const DWORD err = GetLastError();
            std::cerr << "[ERROR] ReadFile failed: " << FormatWindowsError(err) << "\n";
            break;
        }

        if (bytesRead > 0) {
            PrintRx(buffer.data(), bytesRead, cfg.hex, cfg.ascii);
            statsBytes += bytesRead;
        }

        if (cfg.stats) {
            const auto now = std::chrono::steady_clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastStats);
            if (elapsed.count() >= 1000) {
                const double seconds = static_cast<double>(elapsed.count()) / 1000.0;
                const auto bytesPerSec = static_cast<std::uint64_t>(statsBytes / seconds);
                std::cout << "[STATS] " << bytesPerSec << " bytes/sec\n";
                statsBytes = 0;
                lastStats = now;
            }
        }
    }

    CloseHandle(serial);
    std::cout << "[INFO] Exiting.\n";
    return 0;
}
