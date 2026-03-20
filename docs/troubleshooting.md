# Troubleshooting

## Troubleshooting approach

For this manipulator system, diagnosis should follow the control path:

1. operator action in the GUI
2. command transport to the server
3. server-side validation and execution
4. hardware or subsystem response
5. status publication back to the GUI

When the system misbehaves, the main task is to determine where along that path the failure occurred. See [Interfaces](interfaces.md) for the command/status model and [Operation](operation.md) for the normal sequence being diagnosed.

## Common startup issues

### `rimoServer` exits immediately

Likely causes:

- missing or wrong config path
- invalid YAML structure
- missing runtime dependency
- failure while initializing a hardware-facing subsystem

Check:

- server startup logs
- the `--config` argument
- whether the configured endpoints, serial devices, or network targets exist
- whether the same config file worked in the last known-good run

Likely subsystem:

- configuration loading
- server initialization
- hardware interface bring-up

### `rimokunControl` fails to launch

Likely causes:

- missing config path
- GUI runtime dependency problem
- startup failure while initializing transport or state handling

Check:

- GUI startup logs
- display environment
- whether the server is already running if the GUI expects a live system

Likely subsystem:

- GUI startup
- client transport initialization

## GUI is not receiving status updates

Symptoms:

- GUI opens but status fields remain static
- status indicators remain blank, stale, or frozen
- command responses may still work while live updates do not

Likely causes:

- status publication is not running on the server
- GUI status subscription did not initialize correctly
- command and status endpoints are mismatched between client and server
- the server is running but not updating the machine state normally
- status serialization or decode failed

What to check:

- server logs for status publisher startup
- GUI logs for status subscription or decode errors
- configured status address in both `RimoServer` and `RimoClient`
- whether the machine state is changing internally on the server

Likely subsystem:

- server status publisher
- GUI transport worker
- shared status serialization path

## Manipulator is not responding to commands

Symptoms:

- operator issues a motion or control command
- GUI appears active
- no resulting motion or state change is observed

Likely causes:

- command never reached the server
- command was rejected during validation
- command was accepted but execution failed in the hardware-facing layer
- subsystem interlock, fault, or unavailable resource blocked execution

What to check:

- GUI logs for command timeout or send failure
- server logs for command validation or dispatch failure
- subsystem health indicators in the current status
- whether a low-risk recovery or reset command succeeds

Likely subsystem:

- command transport
- command processor
- machine runtime
- motion-control or I/O subsystem

## Tool change failure

Symptoms:

- tool change command is issued
- changer state does not reach the expected result
- command may acknowledge without a matching status transition

Likely causes:

- tool changer command accepted but not completed
- missing or inconsistent feedback from tool-related sensors
- preconditions for tool change were not satisfied
- I/O path or actuator path failed during execution

What to check:

- server logs for command acceptance and execution errors
- status updates related to tool changer state
- any relevant I/O or control panel errors
- whether the manipulator was in the required position before the action

Likely subsystem:

- tool changer execution logic
- digital I/O path
- state feedback path

## Clamp is not engaging or releasing

Symptoms:

- clamp-related action is issued
- clamp state does not change as expected
- sensor indications remain contradictory or unchanged

Likely causes:

- clamp actuation command was not executed
- related output signal did not change
- sensor feedback did not update
- physical or subsystem-level clamp fault

What to check:

- server logs around the command time
- current clamp-related status and sensor indicators
- I/O mapping and output configuration
- whether a subsystem fault is already active

Likely subsystem:

- clamp control path
- digital outputs
- sensor feedback inputs

## Inconsistent sensor readings

Symptoms:

- GUI shows state changes that do not match expected machine behavior
- tool, clamp, or safety-related indicators contradict each other
- status oscillates or flickers without a corresponding operator action

Likely causes:

- unstable or noisy input signal
- incorrect input mapping
- stale status being interpreted as live status
- subsystem-specific communication issue

What to check:

- server logs for repeated reconnects or input read failures
- configured input mappings
- whether the inconsistency exists in raw subsystem state or only in the GUI
- whether the issue is isolated to one sensor family or affects multiple signals

Likely subsystem:

- hardware input path
- mapping configuration
- status-building logic

## Configuration mistakes

Frequent configuration-related failures include:

- mismatched command/status addresses between GUI and server
- wrong serial device path
- wrong TCP host or port
- invalid motor or I/O mapping
- environment-specific configuration copied into the wrong deployment

When debugging configuration, revert to the smallest known-good configuration and reapply changes one category at a time.

## Logs to inspect

Start with:

- server startup and runtime logs
- GUI startup and transport logs
- command validation or dispatch failures
- status serialization or decode failures
- hardware communication and subsystem health messages

If your deployment does not yet define standard log destinations, document them locally and add them to this page.

## Step-by-step debugging checklist

1. Confirm GUI and server are running the expected software revision.
2. Confirm both processes use the intended config file.
3. Verify the server started cleanly and initialized its interfaces.
4. Verify the GUI connected and began receiving fresh status.
5. Trigger one low-risk action.
6. Check for command acknowledgement on the GUI side.
7. Check for command receipt and execution logs on the server side.
8. Check whether the expected status transition followed.
9. If acknowledgement exists without state change, focus on execution and feedback.
10. If no acknowledgement exists, focus on transport and server intake first.

## When to stop operations and escalate

Stop normal operation and move to a controlled maintenance workflow when:

- safety-related state is unclear
- the manipulator appears to move inconsistently with the reported status
- clamp or tool state cannot be confirmed
- repeated timeouts or stale status persist
- recovery actions worsen the state instead of clarifying it
