<div class="hero">
  <h1>rimokun Documentation</h1>
  <p><strong>Remote manipulator control for the J-PARC remote-ready primary beamline upgrade.</strong></p>
  <p>`rimokun` coordinates a gantry-style handling system used where direct access is constrained, undesirable, or hazardous. Operators work through a GUI, the server owns command execution and state, and the whole control loop depends on credible feedback from motion, tooling, clamp, and safety subsystems.</p>
  <div class="hero-badges">
    <span class="hero-badge">J-PARC beamline upgrade</span>
    <span class="hero-badge">Remote handling</span>
    <span class="hero-badge">GUI + server architecture</span>
    <span class="hero-badge">Command and status feedback</span>
  </div>
  <div class="cta-row">
    <a class="primary" href="quickstart.md">Open quickstart</a>
    <a class="secondary" href="architecture.md">Study architecture</a>
    <a class="secondary" href="operation.md">Review operation</a>
  </div>
  <div class="brand-strip">
    <img src="assets/images/jparc-logo.png" alt="J-PARC logo">
    <img src="assets/images/kek-logo.png" alt="KEK logo">
    <img src="assets/images/rimokun-logo.png" alt="rimokun logo">
  </div>
</div>

## Mission profile

The manipulator system supports remote work around the beamline upgrade by coordinating:

- gantry motion and positioning
- tool changing associated with vacuum clamp tooling
- vacuum clamp interaction and related I/O
- sensor, safety, and subsystem state monitoring
- operator feedback through a live control station

<div class="metric-strip">
  <div class="metric"><strong>Operators</strong>Use the GUI to command and verify manipulator behavior.</div>
  <div class="metric"><strong>Engineers</strong>Integrate, commission, diagnose, and recover the control system.</div>
  <div class="metric"><strong>Developers</strong>Maintain the GUI, server runtime, shared transport, and tests.</div>
</div>

## System overview

<div class="diagram-frame">
  <img src="assets/images/system-architecture.svg" alt="Diagram showing GUI, server runtime, and hardware layer">
  <div class="figure-note">The system is built around a single authoritative server runtime. Operators act through the GUI, while hardware feedback flows back through the server so that published status reflects actual runtime state rather than optimistic UI assumptions.</div>
</div>

## Why this documentation is organized the way it is

This is not an API-first project. For a beamline manipulator used remotely, the central questions are:

- what state the machine is in now
- whether a command was only accepted or actually completed
- how tooling and clamp operations are confirmed
- how operators recover when feedback is missing or contradictory

Those concerns drive the documentation structure:

<div class="card-grid">
  <div class="card">
    <h3>Architecture</h3>
    <p>How the GUI, server, and hardware-facing subsystems fit together for deterministic control and feedback.</p>
  </div>
  <div class="card">
    <h3>Interfaces</h3>
    <p>How commands and status updates carry operator intent and machine truth across the process boundary.</p>
  </div>
  <div class="card">
    <h3>Operation</h3>
    <p>How beamline bring-up, tool switching, clamp interaction, and safe shutdown should be handled in practice.</p>
  </div>
  <div class="card">
    <h3>Troubleshooting</h3>
    <p>How to localize failures to transport, execution, status publication, tooling, or sensor paths.</p>
  </div>
</div>

## Start here

- [Installation](installation.md) for prerequisites and local setup
- [Quickstart](quickstart.md) for the fastest path to a running GUI and server
- [Architecture](architecture.md) for the control model
- [Operation](operation.md) for bring-up, tool changes, clamp interaction, and shutdown

<div class="callout">
  <strong>Note:</strong> some deployment-specific details are still placeholders, including any formal external protocol publication and production service topology. Those gaps are marked explicitly rather than filled with guessed behavior.
</div>
