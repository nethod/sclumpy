# meshtastic_monitor

`meshtastic_monitor` is a Windows C++17 console tool that opens a Meshtastic serial port and prints incoming bytes.

## Features

- Native Win32 serial API (`CreateFile` on `\\.\COMx`), no external dependencies.
- Command-line options for port, baud rate, hex output, ASCII preview, and periodic throughput stats.
- Graceful shutdown on `Ctrl+C`.
- Non-blocking behavior with serial read timeouts (does not freeze when no data arrives).
- Clear error messages when the port is missing or busy.

## Command line

```powershell
meshtastic_monitor.exe --port COM5 --baud 115200 [--hex] [--ascii] [--stats]
```

Options:

- `--port COMx` (required)
- `--baud <rate>` (optional, default `115200`)
- `--hex` (optional, default enabled)
- `--ascii` (optional)
- `--stats` (optional, prints bytes/sec every second)

Example output:

```text
[OK] Opened COM5 @ 115200
[RX] 64 bytes: 01 7A ... | .z..
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
# Example
.\build\Release\meshtastic_monitor.exe --port COM5 --baud 115200 --ascii --stats
```

If the COM port is wrong or already in use, the app prints a clear error and exits.
