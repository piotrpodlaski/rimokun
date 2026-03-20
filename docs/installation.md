# Installation

## Prerequisites

The repository indicates a C++ project built with CMake, with a Qt-based GUI and additional native dependencies on the server side.

Minimum expected prerequisites:

- CMake 3.28 or newer
- a C++23-capable compiler
- Python 3.11 or newer for documentation tooling
- Qt 6 with `Core`, `Gui`, and `Widgets`
- `pkg-config`
- `libmodbus`
- `libserial`

The build also pulls several C++ dependencies through CMake, including YAML, JSON, logging, and ZeroMQ C++ bindings.

## Repository setup

Clone the repository and enter the project root:

```bash
git clone <your-repository-url>
cd rimokun
```

## Build the application

Create a build directory and configure the project:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

Build the server and GUI:

```bash
cmake --build build
```

## Configuration assumptions

Both `rimoServer` and `rimokunControl` expect a YAML configuration file passed with `--config`. The code currently defaults to:

```text
/etc/rimokunControl.yaml
```

For local development, the repository already includes:

```text
Config/rimokun.yaml
```

Use that file as the starting point unless you have a deployment-specific configuration source.

## Verify the build

After a successful build, confirm that the expected binaries exist in your build output:

- `rimoServer`
- `rimokunControl`

If your generator places binaries in subdirectories, locate them under `build/` and record the actual paths for your environment.

## Verify runtime prerequisites

Before first startup, confirm:

- the configuration file path is correct
- required device or network endpoints are reachable
- the configured serial device and TCP endpoints match your environment
- local IPC endpoints under `/tmp` are permitted on the target system

## Documentation tool installation

To build the docs locally:

```bash
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## Installation verification

A basic verification sequence is:

1. Start the server with the intended config file.
2. Launch the GUI with the same config file.
3. Confirm that the GUI connects and begins receiving status updates.
4. Confirm that a simple command produces a structured response instead of a timeout or connection failure.

If any of these steps fail, continue with [Troubleshooting](troubleshooting.md).
