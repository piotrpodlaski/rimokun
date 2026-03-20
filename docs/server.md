# Server

## Purpose of the server

`rimoServer` is the authoritative runtime for the manipulator system. It coordinates command execution, machine state, and hardware-facing interaction so that the GUI can remain an operator station rather than an independent control authority.

For this beamline application, centralizing control in the server is important. Motion, tooling, clamp interaction, and subsystem recovery all depend on ordered execution and credible feedback.

## Core responsibilities

The server is responsible for:

- loading and validating runtime configuration
- initializing the manipulator-facing subsystems
- receiving commands from the GUI
- validating and dispatching those commands
- maintaining current machine state
- publishing a unified status view back to clients
- shutting the system down in an orderly way

The current codebase centers these responsibilities in the `Machine` runtime and related command/status components.

## Command handling

The server is the only process that should decide whether a requested action is accepted for execution. In practical terms, that means the server:

- receives the command from the GUI
- validates its structure and allowed values
- routes it to the relevant execution path
- returns a structured response to the caller

Examples already visible in the repository include tool changer actions, reset or reconnect commands, and motor diagnostics.

The acceptance response is only part of the story. For manipulator work, the server must also ensure that subsequent status updates reflect whether the requested action actually changed machine state.

## State management

The server maintains the system’s authoritative state by combining:

- internal machine lifecycle state
- subsystem health and connection state
- sensor-derived or hardware-derived state
- command execution results

That state is then published as a unified status object for the GUI. In a remote operation environment, this aggregation is what lets operators reason about the machine from a distance.

## Interface to the hardware layer

The repository shows server-side integration points for subsystems such as:

- motor control
- control panel communication
- digital I/O
- machine status construction
- command processing

These server-side components form the bridge between high-level operator requests and low-level hardware behavior. The server should shield the GUI from hardware-specific execution details while still exposing enough status to support safe operation and recovery.

## Reliability concerns

For this system, reliability is not only about process uptime. It is also about whether the control path behaves in a way operators and engineers can trust.

Important reliability concerns include:

- command ordering
- avoiding ambiguous execution state
- keeping command acknowledgement separate from completion
- publishing timely status after state changes
- making subsystem faults visible quickly

The exact execution guarantees are not yet documented as a formal contract, so this page stays at the structural level rather than overstating behavior.

## Runtime behavior

At startup, the server:

1. loads configuration
2. constructs and wires the machine
3. initializes the relevant subsystems
4. starts command handling and machine processing loops
5. runs until shutdown is requested

The current code handles `SIGINT` and `SIGTERM`, which supports clean shutdown during local runs and under a service manager.

## Logging and observability

The server is the primary place to inspect:

- startup failures
- hardware communication errors
- command validation failures
- execution failures
- status publication problems

If the GUI view and the hardware state appear inconsistent, start with the server logs and current server-published status before assuming a GUI defect.

## Operational notes

Treat `rimoServer` as the source of truth for machine execution. If the GUI disconnects or shows stale data, the server still owns:

- the command intake path
- the current machine coordination state
- the most useful diagnostics for recovery

## Related pages

- [Architecture](architecture.md) for the end-to-end control path
- [Interfaces](interfaces.md) for command and status semantics
- [Troubleshooting](troubleshooting.md) for runtime failure analysis
