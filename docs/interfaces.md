# Interfaces

## Overview

The interfaces between `rimokunControl` and `rimoServer` carry the operational contract for the manipulator system. They are not generic integration endpoints. They are the path by which operator intent becomes machine action and by which machine feedback returns to the operator station.

For this system, the interface layer has two responsibilities:

- carry commands from the GUI to the server
- carry status from the server back to the GUI

The repository currently shows this split as a command request/response channel and a separate status publication channel. See [Architecture](architecture.md) for the full control path.

## Command classes

Commands should be understood in terms of manipulator operation rather than software abstractions. In this system, a command may correspond to:

- a motion or positioning request for the gantry manipulator
- a tool changer action associated with vacuum clamp tooling
- a clamp-related action or recovery step
- a subsystem reset, reconnect, or diagnostic request

The current code and tests already show command categories such as tool changer actions, reset commands, and motor diagnostics. Additional project-specific command classes should be documented here only when they are intentionally defined and versioned.

## Status classes

Status updates represent the server’s current view of the manipulator and related subsystems. For an operator, status is the primary confirmation that the requested action has translated into real machine behavior.

At a practical level, the status model is expected to cover areas such as:

- position or motion feedback
- tool state
- vacuum clamp-related state
- subsystem health
- sensor readings and safety indications
- fault, warning, or degraded-operation state

The current shared status types in the repository already include motor state, tool changer state, component health, joystick-related state, and safety information.

## Direction of communication

The interface directions are intentionally asymmetric:

- commands flow from GUI to server
- command responses flow from server to the requesting GUI client
- status flows from server to subscribed GUI clients

This matters operationally. Operators should not rely on polling alone to determine whether the manipulator has reached the requested state. The response path and the status path serve different purposes.

## Command lifecycle

A useful way to reason about the interface is as a command lifecycle:

1. The operator issues a command in the GUI.
2. The GUI transmits the request to the server.
3. The server acknowledges the request or rejects it as invalid.
4. If accepted, the server dispatches the request to the relevant subsystem logic.
5. The command is executed, partially executed, or fails.
6. The resulting machine state is reflected through subsequent status updates.

In operational terms, that lifecycle often looks like:

- issued
- acknowledged
- executing
- completed or failed

The exact serialized representation of those phases is not yet documented as a formal protocol and should not be guessed here.

## Why acknowledgement is not completion

For a remote manipulator, command acceptance is not the same as task completion. A response that indicates the server accepted a request only means that:

- the request reached the server
- it passed validation
- it was handed to the execution path

Completion must be confirmed using machine state and follow-up status. This is especially important for:

- motion commands
- tool change operations
- clamp engagement or release

## State synchronization challenges

The interface layer has to bridge an asynchronous physical system and a user-facing GUI. Common synchronization challenges include:

- the GUI may still show the previous state while the server is executing a command
- hardware feedback may lag behind command acceptance
- a subsystem may fault after a command was accepted
- the GUI may lose status updates without losing the whole process
- the server may restart while the GUI remains open

The documentation and the GUI behavior should make these distinctions visible rather than hiding them behind a single “success” indicator.

## Motion-related command handling

Motion or positioning commands require particular care because the manipulator is used remotely. The interface should support a clear distinction between:

- request accepted
- motion in progress
- motion completed
- motion blocked or faulted

The exact motion primitives are intentionally not specified here because the published command contract is not yet formalized.

## Tool change and clamp interaction

Tooling actions are a central part of this system’s domain. The interface must support at least the concept of:

- commanding a tool change or clamp-related action
- confirming that the request was accepted by the server
- observing the resulting tool or clamp state through status updates
- reporting timeout, interlock, or execution failures clearly

Because these actions can involve remote vacuum clamp tooling, status confirmation is as important as the command itself.

## Timeout and failure handling

The current codebase already shows timeout-aware command dispatch behavior and communication timeouts in the client transport. Operationally, the interface layer should distinguish between:

- invalid command content
- server-side execution failure
- no response from the command path
- loss of status updates
- status that is present but inconsistent with the requested action

Those are different failure modes and should lead engineers to different subsystems during diagnosis.

## Failure handling concepts

Use the following conceptual split when documenting or debugging interface behavior:

- transport failure: the GUI cannot exchange messages with the server
- validation failure: the server received the request but rejected it
- execution failure: the server accepted the request but the subsystem could not complete it
- state feedback failure: the subsystem may have acted, but the GUI did not receive reliable confirmation

This distinction is essential in a remote-handling workflow because recovery actions depend on knowing where the failure occurred.

## Current transport model

The shared transport wrappers indicate a ZeroMQ-based model:

- command channel: request/reply
- status channel: publish/subscribe

The current default configuration uses local IPC endpoints:

- `ipc:///tmp/rimoCommand`
- `ipc:///tmp/rimoStatus`

Treat these as implementation details of the current deployment model, not as a guaranteed external protocol contract.

## Example structural placeholders

The following examples reflect patterns already visible in the codebase. They are examples of shape and intent, not a complete protocol specification.

Example tool changer command:

```json
{
  "type": "toolChanger",
  "position": "Left",
  "action": "Open"
}
```

Example subsystem reset command:

```json
{
  "type": "reset",
  "system": "ControlPanel"
}
```

Example command response shape:

```json
{
  "status": "OK",
  "message": ""
}
```

## Related pages

- [Architecture](architecture.md) for end-to-end control and feedback flow
- [Operation](operation.md) for how operators use commands and status during normal work
- [Troubleshooting](troubleshooting.md) for diagnosing command, status, and synchronization failures
