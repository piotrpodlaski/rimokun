# Architecture

## System overview

`rimokun` controls a gantry-style remote manipulator for the J-PARC beamline upgrade. Operators work through the GUI. The server executes commands and publishes machine state back to the GUI.

The basic runtime model is:

1. `rimoServer` starts and initializes the manipulator-facing subsystems.
2. The server exposes command and status interfaces.
3. `rimokunControl` connects as the operator client.
4. Operators issue commands through the GUI.
5. The server executes or rejects those commands and publishes current state.

This split keeps control logic in one place and makes the server the source of truth for machine state.

<div class="diagram-frame">
  <img src="assets/images/system-architecture.svg" alt="High-level architecture diagram for rimokun">
  <div class="figure-note">The GUI is the operator surface, the server is the execution authority, and the hardware-facing layer provides the feedback needed to confirm motion, tool changes, and clamp state.</div>
</div>

## Major components

### GUI layer

`rimokunControl` is the operator station. It is used for:

- observing manipulator state
- issuing motion-related commands
- controlling tool changer actions
- monitoring subsystem health and safety-related indications

The GUI shows published state and sends operator requests. It does not own runtime state.

### Server layer

`rimoServer` coordinates:

- command intake and validation
- machine lifecycle
- interaction with motor control, control panel, and I/O subsystems
- aggregation of status into a single machine view
- publication of operator-visible state

The `Machine` class is the central runtime object.

### Hardware-facing layer

Below the server are the hardware-facing subsystems, including:

- gantry and axis motion control
- digital I/O interaction
- control panel communication
- tool changer actuation
- vacuum clamp-related signal handling
- sensor and safety signal monitoring

## Control path

The control path is:

- operators interact with the GUI
- the GUI sends requested actions to the server
- the server decides whether and how those actions are executed
- the hardware-facing subsystems produce feedback
- the server converts that feedback into published system state
- the GUI renders that state back to the operator

## Status path

The status path is:

- hardware and subsystem state is read inside the server
- the server builds a machine status snapshot
- status is published to clients
- the GUI updates the operator view

## Command flow

Command flow is:

1. An operator selects an action in the GUI.
2. The GUI creates a command representing that request.
3. The command is sent to the server through the command interface.
4. The server validates the request against expected structure and allowed values.
5. The server dispatches the request to the relevant machine or subsystem logic.
6. The server returns an acknowledgement or error response to the GUI.
7. Ongoing state changes are reflected through subsequent status updates.

Typical command categories include:

- manipulator motion or positioning actions
- tool changer actions
- subsystem reset or recovery actions
- maintenance or diagnostic commands

<div class="diagram-frame">
  <img src="assets/images/command-lifecycle.svg" alt="Diagram showing command lifecycle from issued to completed or failed">
  <div class="figure-note">A response that says a command was accepted does not by itself prove that the manipulator reached the requested state. Completion is confirmed through subsequent status updates and subsystem feedback.</div>
</div>

## Safety considerations

Remote handling makes a few things especially important:

- operators may not have direct visual or physical confirmation beyond the provided feedback channels
- motion, tool change, and clamp interaction all depend on reliable state awareness
- command execution should prefer safe rejection over ambiguous behavior
- faulted or stale state should be visible quickly enough to support safe intervention

## Deployment and communication model

The current repository uses local IPC endpoints by default. `Config/rimokun.yaml` defines:

- status address: `ipc:///tmp/rimoStatus`
- command address: `ipc:///tmp/rimoCommand`

This indicates a same-host control station model in the current setup.

## Related pages

- [Interfaces](interfaces.md) for command and status behavior
- [GUI](gui.md) for operator workflows
- [Server](server.md) for runtime coordination responsibilities
- [Operation](operation.md) for bring-up and shutdown guidance
- [Class Reference](classes.md) for the main runtime classes
