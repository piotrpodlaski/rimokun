# Rimokun — Improvement Suggestions

## Critical

### Emergency stop is unimplemented
**File:** `Server/src/Machine.cpp:812`

`handleEmergencyStopCommand` only logs `"not yet implemented"` and returns. The full command path exists, including the GUI button and command dispatch, but the handler does not disable or stop any motor.

At minimum this should disable all drives with `_motorControl.setAllEnabled(false)`. A stronger implementation would also issue stop pulses and latch an emergency-stop state that blocks subsequent motion commands until explicitly cleared.

---

## High Priority

### `_busMutex` held across a 30 ms sleep during pulse operations — known hardware constraint
**Files:** `Server/src/Motor.cpp:541-556`, `Server/src/MotorControl.cpp`

`pulseDriverInputFlag` sleeps between the assert and deassert Modbus writes. Because all motors share one bus, the mutex must remain held across the entire pulse. Releasing it would allow interleaved traffic to another slave and corrupt the pulse.

This means pulse operations impose a real bus stall and visible status jitter. It is not a normal software defect, but it should remain documented as an expected hardware limitation.

---

## Medium Priority

### `RimoServer::publish` still builds an intermediate `nlohmann::json` DOM
**File:** `Utilities/include/RimoServer.hpp:38-41`

```cpp
nlohmann::json::to_msgpack(nlohmann::json(robot), _statusPayload);
```

This constructs a large temporary JSON object only to serialize it immediately. Reusing a direct serializer path or a more targeted buffer-building approach would reduce allocations and per-status overhead.

### `VMotorStats` map still uses raw non-owning pointers
**Files:** `Utilities/include/VMotorStats.hpp:25`, `GUI/include/MainWindow.hpp:46`

The code now documents that the pointers are non-owning, which helps, but the type is still `std::map<utl::EMotor, VMotorStats*>`. This remains fragile under refactoring because the ownership model is implicit rather than encoded in the type.

---

## No Longer Applicable

- `GuiCommand` duplication between GUI and server command payloads has been removed.
- Gantry joystick control now emits intents for both `ZLeft` and `ZRight`.
- `Machine::runOneCycle` exception recovery now shares the `resetComponentsToError()` path instead of duplicating full recovery logic.
- Homing now uses a server-owned session state machine with sequential multi-axis progression, timeout handling, and a persistent GUI status dialog.
- "Home all" now requires confirmation before dispatch.
