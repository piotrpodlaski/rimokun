# Class Reference

This page is a short guide to the main classes that shape the current GUI and server design. It is not a full generated API reference. The goal is to help developers find the core control path quickly.

## GUI classes

### `MainWindow`

File: `GUI/include/MainWindow.hpp`

Main Qt window for the operator console.

Responsibilities:

- owns the main GUI layout
- opens operator panels such as joystick, motor, and Contec views
- receives robot status updates
- connects presenters and visual state to the update flow

Use this class as the main entry point when changing operator-facing behavior.

### `Updater`

File: `GUI/include/Updater.hpp`

Bridge between the GUI and the transport worker.

Responsibilities:

- starts and stops the background update thread
- sends GUI commands to the transport worker
- receives status updates and responses
- tracks pending requests and reports connection problems

This class is the main coordination point for GUI-side communication.

### `GuiStateStore`

File: `GUI/include/GuiStateStore.hpp`

Small state container for the latest server status and connection state.

Responsibilities:

- stores the most recent `RobotStatus`
- tracks whether the GUI is connected
- emits Qt signals when status or connection state changes

### `RimoTransportWorker`

File: `GUI/include/RimoTransportWorker.hpp`

Background worker that exchanges messages with the server.

Responsibilities:

- runs the client communication loop
- receives published status updates
- sends queued commands
- reports responses and connection loss back to the GUI

## Server classes

### `Machine`

File: `Server/include/Machine.hpp`

Central runtime class for the server.

Responsibilities:

- wires server-side subsystems together
- initializes and shuts down the machine runtime
- owns command processing and status publication threads
- runs control-loop work
- dispatches command types such as tool change, reconnect, diagnostics, and emergency stop

This is the first class to read when changing server behavior.

### `MachineCommandServer`

File: `Server/include/MachineCommandServer.hpp`

Command intake loop for the server.

Responsibilities:

- receives commands from the command channel
- passes commands into the dispatch function
- returns structured responses to the client

### `MachineStatusBuilder`

File: `Server/include/MachineStatusBuilder.hpp`

Builds the published `RobotStatus` snapshot.

Responsibilities:

- reads component and I/O state
- converts low-level values into operator-facing status
- updates status fields such as motors, tool changers, and component health
- publishes the status snapshot

### `MachineController`

File: `Server/include/MachineController.hpp`

Control-loop helper for motion and I/O behavior.

Responsibilities:

- runs control-loop tasks
- applies control policy decisions
- handles tool changer commands
- coordinates motor and I/O operations through injected callbacks

## Shared transport classes

### `utl::RimoClient<T>`

File: `Utilities/include/RimoClient.hpp`

Client-side transport wrapper used by the GUI side.

Responsibilities:

- connects to the status and command endpoints
- receives published status
- sends commands and waits for responses

### `utl::RimoServer<T>`

File: `Utilities/include/RimoServer.hpp`

Server-side transport wrapper used by the runtime.

Responsibilities:

- publishes status snapshots
- receives command messages
- returns command responses

## Related pages

- [Architecture](architecture.md) for the full system structure
- [Interfaces](interfaces.md) for command/status behavior
- [GUI](gui.md) for the operator-facing side
- [Server](server.md) for the runtime view
