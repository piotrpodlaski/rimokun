<div class="hero">
  <h1>rimokun Documentation</h1>
  <p><strong>Remote manipulator control for the J-PARC remote-ready primary beamline upgrade.</strong></p>
  <p>`rimokun` controls a gantry-style remote handling system with a GUI, a server backend, and command/status links between them. It is built for motion control, tool changing, vacuum clamp work, and feedback-driven operation in constrained environments.</p>
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

The system is used to coordinate:

- gantry motion and positioning
- tool changing associated with vacuum clamp tooling
- vacuum clamp interaction and related I/O
- sensor, safety, and subsystem state monitoring
- operator feedback through a live control station

<div class="metric-strip">
  <div class="metric"><strong>Operators</strong>Run the manipulator from the GUI and verify state.</div>
  <div class="metric"><strong>Engineers</strong>Commission, maintain, and troubleshoot the system.</div>
  <div class="metric"><strong>Developers</strong>Work on the GUI, server, transport, and tests.</div>
</div>

## System overview

<div class="diagram-frame">
  <img src="assets/images/system-architecture.svg" alt="Diagram showing GUI, server runtime, and hardware layer">
  <div class="figure-note">The system is built around a single authoritative server runtime. Operators act through the GUI, while hardware feedback flows back through the server so that published status reflects actual runtime state rather than optimistic UI assumptions.</div>
</div>

## Main sections

<div class="card-grid">
  <div class="card">
    <h3>Architecture</h3>
    <p>How the GUI, server, and hardware-facing subsystems fit together.</p>
  </div>
  <div class="card">
    <h3>Interfaces</h3>
    <p>How commands and status move between GUI and server.</p>
  </div>
  <div class="card">
    <h3>Operation</h3>
    <p>How to start, use, and stop the system safely.</p>
  </div>
  <div class="card">
    <h3>Troubleshooting</h3>
    <p>Where to look when commands, status, tools, or clamps do not behave correctly.</p>
  </div>
  <div class="card">
    <h3>Class Reference</h3>
    <p>A short guide to the main GUI and server classes in the codebase.</p>
  </div>
</div>

## Start here

- [Installation](installation.md) for prerequisites and local setup
- [Quickstart](quickstart.md) for the fastest path to a running GUI and server
- [Architecture](architecture.md) for the control model
- [Operation](operation.md) for bring-up, tool changes, clamp interaction, and shutdown
- [Class Reference](classes.md) for key source-level classes

<div class="callout">
  <strong>Note:</strong> some deployment-specific details are still placeholders, including any formal external protocol publication and production service topology. Those gaps are marked explicitly rather than filled with guessed behavior.
</div>
