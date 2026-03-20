# Server

## Purpose of the server

`rimoServer` is the authoritative runtime for the manipulator system. It coordinates command execution, machine state, and hardware-facing interaction so that the GUI can remain an operator station rather than an independent control authority.

For this system, centralizing control in the server keeps command ordering, state updates, and recovery logic in one place.

## Core responsibilities

The server is responsible for:

- loading and validating runtime configuration
- initializing the manipulator-facing subsystems
- receiving commands from the GUI
- validating and dispatching those commands
- maintaining current machine state
- publishing a unified status view back to clients
- shutting the system down in an orderly way

The main coordination point is the `Machine` runtime.

## Command handling

The server decides whether a requested action is accepted for execution. It:

- receives the command from the GUI
- validates its structure and allowed values
- routes it to the relevant execution path
- returns a structured response to the caller

Examples in the current codebase include tool changer actions, reset or reconnect commands, and motor diagnostics.

## State management

The server maintains system state by combining:

- internal machine lifecycle state
- subsystem health and connection state
- sensor-derived or hardware-derived state
- command execution results

That state is published to the GUI as a unified status object.

## Interface to the hardware layer

Server-side subsystems include:

- motor control
- control panel communication
- digital I/O
- machine status construction
- command processing

## Reliability concerns

Main reliability concerns are:

- command ordering
- avoiding ambiguous execution state
- keeping command acknowledgement separate from completion
- publishing timely status after state changes
- making subsystem faults visible quickly

## Runtime behavior

At startup, the server:

1. loads configuration
2. constructs and wires the machine
3. initializes the relevant subsystems
4. starts command handling and machine processing loops
5. runs until shutdown is requested

The current code handles `SIGINT` and `SIGTERM` for clean shutdown.

## Logging and observability

The server is the primary place to inspect:

- startup failures
- hardware communication errors
- command validation failures
- execution failures
- status publication problems

If GUI state and hardware behavior disagree, start with server logs and server-published status.

## Related pages

- [Architecture](architecture.md) for the end-to-end control path
- [Interfaces](interfaces.md) for command and status semantics
- [Troubleshooting](troubleshooting.md) for runtime failure analysis
- [Class Reference](classes.md) for `Machine`, `MachineCommandServer`, `MachineStatusBuilder`, and `MachineController`
