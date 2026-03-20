# GUI

## Purpose of the GUI

`rimokunControl` is the operator console for the gantry-style remote manipulator. Operators use it to issue motion and tooling actions, observe current machine state, and detect when the manipulator or associated subsystems are no longer behaving as expected.

The GUI is not the authority for machine state. It depends on the server for command execution and status publication.

## What operators do in the GUI

In normal work, operators use the GUI to:

- confirm the system is connected and publishing live status
- observe current manipulator position and subsystem state
- issue motion, tool change, or recovery commands
- monitor tool and clamp-related feedback after each action
- respond to communication, subsystem, or fault indications

This is an operational workflow, not just a collection of widgets.

## Operator workflow

### 1. Confirm readiness

Before commanding the manipulator, the operator should confirm in the GUI that:

- the server connection is active
- status is updating
- subsystem health indicators are credible
- the machine is in the expected starting state

### 2. Issue a command

The operator selects an action in the GUI, such as:

- a manipulator movement
- a tool changer action
- a subsystem reset or recovery action

The GUI sends that request to the server and waits for an explicit response.

### 3. Watch feedback

After a command is accepted, the operator should watch:

- position or motion-related state
- tool state
- clamp-related state
- warning or fault indicators

The GUI should be used to confirm actual state transitions, not just command submission.

### 4. Respond to errors

If the GUI reports a timeout, connection problem, or unexpected status:

- stop issuing additional commands
- determine whether the problem is transport, execution, or feedback related
- move to the recovery guidance in [Troubleshooting](troubleshooting.md)

## Main responsibilities

The current codebase indicates that the GUI is responsible for:

- presenting the main operator window
- showing machine and subsystem state
- collecting operator input
- sending commands to the server
- reacting to responses and connection problems
- updating local view state from streamed status messages

## Major screens and functions

Based on the current source tree, the GUI includes support for:

- the main control window
- joystick-related interaction
- motor panels and motor statistics
- tool changer controls
- Contec-related views

Those elements should eventually be documented with screenshots and operator procedures specific to the beamline workflow.

## Operator-facing behavior

The GUI should make the following distinctions clear:

- server reachable vs. server unreachable
- command accepted vs. command completed
- live status vs. stale status
- subsystem fault vs. user-input error

Those distinctions are important in a remote-handling environment where the operator may not have direct access to the hardware.

## State update model

The GUI receives continuous status updates and also performs synchronous command exchanges. That means operators see two different kinds of feedback:

- command responses that indicate whether the server accepted or rejected a request
- status updates that show the current machine state

Operators should use both together. A successful response is not sufficient confirmation for motion completion, tool change success, or clamp engagement.

## Related pages

- [Interfaces](interfaces.md) for command and status behavior
- [Operation](operation.md) for the normal operating sequence
- [Troubleshooting](troubleshooting.md) for operator-visible failure scenarios
