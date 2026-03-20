# Development

## Repository structure

The repository is organized around the system boundary:

- `GUI/`: Qt application, UI forms, presenters, and transport integration
- `Server/`: machine runtime, command handling, and hardware-oriented logic
- `Utilities/`: shared configuration, transport, logging, and common types
- `Config/`: example runtime configuration
- `tests/`: unit and integration tests
- `docs/`: documentation site sources

## Local development workflow

A practical workflow is:

1. configure and build with CMake
2. run targeted tests during development
3. start the server with a local config
4. run the GUI against the same config
5. verify command and status behavior before submitting changes

## Build commands

Example local build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

## Running tests

CTest is enabled in the root CMake configuration. A typical test run is:

```bash
ctest --test-dir build --output-on-failure
```

Use narrower test targets while iterating if needed.

## Running docs locally

Install the documentation dependencies:

```bash
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

Start the MkDocs development server:

```bash
mkdocs serve
```

Build the static site:

```bash
mkdocs build
```

## Documentation expectations

Documentation changes should accompany:

- changes to configuration structure
- new operator workflows
- interface behavior changes
- deployment or startup changes
- troubleshooting lessons that should become permanent guidance

For key runtime classes, also update [Class Reference](classes.md).

## Testing expectations

Prefer tests that protect system behavior, especially around:

- command validation
- command dispatch behavior
- status construction
- transport integration points
- configuration parsing

## Contribution guidance

Contributors should:

- keep GUI, server, and shared-interface changes consistent
- update docs when runtime behavior changes
- avoid undocumented protocol drift between GUI and server
- add or update tests for behavioral changes
