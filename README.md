# meshtastic_monitor

`meshtastic_monitor` is a Windows C++17 console tool that opens a Meshtastic serial port and prints incoming data.

## Features

- Native Win32 serial API (`CreateFile` on `\\.\COMx`), no external dependencies.
- **Default readable log mode** with line buffering (`\r\n`) and `[LOG] ...` output.
- ANSI color/format cleanup by default (`--strip-ansi` on), with `--no-strip-ansi` to preserve escape codes.
- Optional raw receive mode (`--raw`) and optional hex dump (`--hex`).
- Optional throughput stats (`--stats`) once per second.
- Graceful shutdown on `Ctrl+C`.
- Non-blocking behavior with serial read timeouts (does not freeze when no data arrives).
- Clear error messages when the port is missing or busy.

## Command line

```powershell
meshtastic_monitor.exe --port COM5 --baud 115200 [--raw] [--hex] [--strip-ansi] [--no-strip-ansi] [--stats]
```

Options:

- `--port COMx` (required)
- `--baud <rate>` (optional, default `115200`)
- `--raw` (optional, prints raw receive chunks instead of parsed log lines)
- `--hex` (optional, prints hex dump of bytes; most useful with `--raw`)
- `--strip-ansi` (optional, default on; removes ANSI escape sequences)
- `--no-strip-ansi` (optional, preserves ANSI escape sequences)
- `--stats` (optional, prints bytes/sec every second)

Example output (default mode):

```text
[OK] Opened COM5 @ 115200
[LOG] DEBUG | 08:22:20 343601 [Router] Routing sniffing (...)
[LOG] INFO | ...
[STATS] 1024 bytes/sec
```

## Build with CMake (Visual Studio generator)

From a **Developer PowerShell for Visual Studio**:

```powershell
cd <repo-path>
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Binary output:

- `build\Release\meshtastic_monitor.exe`

## Build with Visual Studio solution

1. Open `meshtastic_monitor.sln` in Visual Studio 2022 (or newer).
2. Select configuration (`Debug` or `Release`) and platform (`x64` or `Win32`).
3. Build the `meshtastic_monitor` project.

## Run

```powershell
# Default readable log mode (ANSI stripped)
.\build\Release\meshtastic_monitor.exe --port COM5 --baud 115200

# Raw mode with hex dump
.\build\Release\meshtastic_monitor.exe --port COM5 --baud 115200 --raw --hex
```

If the COM port is wrong or already in use, the app prints a clear error and exits.
