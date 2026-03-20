# Operation

## Operational context

`rimokun` is operated as a remote manipulator control system for the J-PARC primary beamline upgrade. Normal operation is not just “starting an application.” It is a bring-up sequence that establishes control, feedback, and confidence that the manipulator, tooling, and associated subsystems are in a known state before any motion or clamp-related action is attempted.

Operators should treat command acceptance and machine state confirmation as separate checks. For command/status behavior, see [Interfaces](interfaces.md). For failure recovery, see [Troubleshooting](troubleshooting.md).

## System bring-up

Before starting the software:

1. confirm the intended configuration file and revision
2. confirm the target hardware environment is available
3. confirm any required maintenance or access conditions are satisfied
4. confirm the manipulator area is in the expected starting condition

Then bring the software up in a consistent order:

1. start `rimoServer`
2. confirm server initialization completed without configuration or hardware communication errors
3. confirm the command and status interfaces are available
4. start `rimokunControl`
5. confirm the GUI is receiving live status rather than a stale display

Do not begin motion or tooling operations until the GUI shows credible current state.

## Initial state checks

After bring-up, operators or engineers should verify:

- manipulator status is updating
- subsystem health indicators are normal or understood
- safety-related signals are in the expected state
- no unresolved command or communication faults are present
- tool and clamp-related state matches the physical starting condition

If any of these checks fail, stop at diagnosis rather than continuing into operation.

## Normal operation flow

A typical operating sequence is:

1. review the current manipulator state in the GUI
2. issue a motion or tooling command
3. confirm the server accepted the command
4. monitor status updates until the requested state is reached or the action faults
5. record or report any abnormal behavior before proceeding to the next action

This matters in remote handling because the operator often depends on software feedback to understand whether the requested action actually happened.

## Motion and positioning

When moving the manipulator:

- verify the current state before issuing the command
- issue one deliberate action at a time unless the procedure explicitly allows otherwise
- watch for changes in position or motion-related indicators
- treat missing or contradictory feedback as a stop condition

If the GUI reports command acceptance but the expected position feedback does not follow, shift immediately into diagnosis.

## Tool switching

Tool changes should be treated as a stateful operation rather than a single button press. A practical sequence is:

1. confirm the manipulator is in the required position for tool change
2. confirm the current tool or changer state is as expected
3. issue the tool change command from the GUI
4. monitor acknowledgement from the server
5. watch subsequent status until the tool state settles into the expected condition

Do not assume that server acknowledgement alone means the tool change completed successfully. Use the feedback path to confirm the result.

## Interacting with vacuum clamps

Clamp-related actions should follow the same discipline as tool changes:

1. confirm the manipulator and tooling are in the correct precondition
2. issue the clamp-related action from the GUI
3. verify the command was accepted
4. confirm clamp-related status and sensor feedback move to the expected state

If clamp state does not update as expected, or sensor indications are inconsistent, stop normal operation and investigate before repeating the action.

## Monitoring during operation

During normal work, operators should continuously watch:

- position or motion feedback
- tool state
- clamp-related state
- subsystem health indicators
- safety-related status
- communication or timeout errors

The GUI should be treated as a live operational console, not just a command entry point.

## Safe shutdown

For an orderly shutdown:

1. stop active manipulator actions
2. confirm the machine is in an acceptable end state for shutdown
3. close or idle the GUI according to local procedure
4. stop `rimoServer` cleanly
5. confirm shutdown did not leave unresolved actuator or subsystem faults

The current server code handles termination signals, which supports controlled shutdown under a terminal session or service manager.

## Operational best practices

- use one validated configuration per environment
- keep the GUI and server on the same configuration revision
- prefer explicit operator confirmation over optimistic assumptions
- treat repeated timeout or stale-status events as operational issues
- record known-good bring-up, tool change, and shutdown procedures for the beamline team

## Placeholder operational content to add later

Add exact project procedures here when available:

- commissioning checklists
- shift handover notes
- alarm response workflow
- service-manager startup units
- maintenance mode and recovery procedures
