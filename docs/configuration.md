# Configuration

## Configuration philosophy

`rimokun` is configured as a system, not as a loose collection of unrelated modules. Configuration should define:

- how the GUI and server find each other
- how the server reaches machine-facing resources
- how control and safety-related mappings are interpreted
- how runtime behavior is tuned for the target environment

Prefer explicit, version-controlled configuration files over ad hoc local edits.

## Where configuration lives

The repository includes a current example configuration at:

```text
Config/rimokun.yaml
```

The binaries accept a config path using `--config`. The current code defaults to:

```text
/etc/rimokunControl.yaml
```

That default path should be treated as a deployment convention, not a requirement for local development.

## Configuration categories

The current configuration file already groups settings by subsystem. Important sections include:

- `RimoServer`: command and status endpoint addresses
- `RimoClient`: client-side command and status endpoint addresses
- `Contec`: I/O connection settings
- `ControlPanel`: communication and input processing behavior
- `MotorControl`: transport, timeouts, motor mapping, and axis behavior
- `Machine`: signal mappings, loop timing, and higher-level machine behavior
- GUI-specific sections such as `RobotVisualisation`

## Example structure

This simplified example shows the shape of the current configuration model:

```yaml
classes:
  RimoServer:
    statusAddress: "ipc:///tmp/rimoStatus"
    commandAddress: "ipc:///tmp/rimoCommand"

  RimoClient:
    statusAddress: "ipc:///tmp/rimoStatus"
    commandAddress: "ipc:///tmp/rimoCommand"

  Contec:
    ipAddress: "10.1.1.101"
    port: 502
    slaveId: 0

  ControlPanel:
    comm:
      type: "serial"
      serial:
        port: "/dev/ttyACM0"
        baudRate: "BAUD_115200"

  MotorControl:
    transport:
      type: "rawTcpRtu"
      tcp:
        host: "10.1.1.102"
        port: 4002

  Machine:
    inputMapping:
      safetyON: 8
    outputMapping:
      toolChangerLeft: 0
```

Replace values with deployment-specific settings and keep the overall structure aligned with the code.

## Safe change guidance

When changing configuration:

- change one subsystem at a time
- keep GUI and server interface endpoints consistent
- validate I/O mappings carefully before deployment
- document why a non-default timeout or motion parameter was changed
- test changes in a development or maintenance window first

## Settings that deserve extra care

The following categories can have operational impact:

- machine I/O mappings
- control panel serial settings
- motor transport settings
- timing and timeout values
- interface addresses if multiple processes depend on them

## Configuration management recommendations

Use separate files for:

- local development
- integration or staging
- production

Suggested naming pattern:

```text
Config/rimokun.dev.yaml
Config/rimokun.staging.yaml
Config/rimokun.prod.yaml
```

This naming scheme is a placeholder recommendation and can be replaced by your deployment standard.

## Validation checklist

Before applying a configuration change:

1. Confirm the file parses cleanly as YAML.
2. Confirm required sections still exist.
3. Confirm GUI and server endpoint values match.
4. Confirm referenced devices, ports, and hosts exist in the target environment.
5. Restart the affected process and verify logs for configuration load success.
