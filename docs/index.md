# rimokun Documentation

`rimokun` is the control software for a gantry-style remote manipulator system developed for the J-PARC remote-ready primary beamline upgrade. It is intended for remote handling tasks in constrained or hazardous environments where operators must control motion, tooling, and beamline-adjacent equipment without relying on direct physical access.

This documentation describes the system as it is operated in practice: a desktop GUI used by operators, a server backend that coordinates execution and state, and command/status interfaces that connect operator intent to machine behavior.

## What this system does

The manipulator system supports remote handling activities around the beamline upgrade by coordinating:

- gantry motion and positioning
- tool changing mechanisms associated with vacuum clamp tooling
- interaction with remote vacuum clamps
- sensor and safety signal monitoring
- operator feedback through a live GUI

The software is designed around the assumption that the controlled hardware may be difficult to access directly during operation or recovery. Feedback, clear state reporting, and predictable command handling are therefore central design concerns.

## Who this documentation is for

- Operators who use the GUI to position the manipulator, change tools, and monitor machine state
- Engineers responsible for integration, commissioning, maintenance, and operational support
- Developers working on the GUI, server runtime, shared transport, and tests

## System at a glance

The deployed system is organized into three main parts:

- `rimokunControl`: the operator GUI
- `rimoServer`: the server process that coordinates command execution, hardware interaction, and state publication
- command/status interfaces: the communication layer between GUI and server

At runtime, operators act through the GUI. The GUI sends commands to the server, the server validates and executes them against the machine and hardware-facing subsystems, and the resulting status is published back to the GUI so operators can confirm actual state rather than assumed state.

## Why the architecture matters

For a remote manipulator used in a beamline environment, the software cannot behave like a loose collection of independent tools. The control path and the feedback path must stay aligned:

- motion and actuation commands must be routed through a single coordinating runtime
- status must reflect the actual machine state as seen by the server and hardware layer
- faults, communication loss, and stale feedback must be visible to operators quickly

The details of that design are covered in [Architecture](architecture.md) and [Interfaces](interfaces.md).

## Major capabilities

- operator control through a dedicated desktop GUI
- centralized server-side command handling and machine coordination
- live publication of manipulator, tool, and subsystem status
- YAML-based runtime configuration
- automated tests for command handling, state updates, and integration points

## Repository layout

The repository reflects the system boundary directly:

- `GUI/` contains the Qt operator interface
- `Server/` contains the machine runtime and command handling logic
- `Utilities/` contains shared configuration, transport, logging, and common state types
- `Config/rimokun.yaml` provides the current example runtime configuration

## Start here

- [Installation](installation.md) for prerequisites and setup
- [Quickstart](quickstart.md) for the fastest path to a local run
- [Operation](operation.md) for day-to-day bring-up and shutdown guidance
- [Troubleshooting](troubleshooting.md) for common runtime failures

## Placeholder notes

Some deployment details are still project placeholders, including formal service definitions, exact production topology, and any externally published interface contract. Those gaps are marked explicitly rather than filled with guessed behavior.
