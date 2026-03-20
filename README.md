# rimokun

`rimokun` is the control software for a gantry-style remote manipulator system used in the J-PARC remote-ready primary beamline upgrade. It is intended for remote handling work in constrained or hazardous environments where operators need reliable control, clear feedback, and safe recovery paths.

The system includes:

- a Qt-based GUI
- a server runtime
- command and status interfaces between them

It should be understood and documented as an application stack, not as an API-first library.

## What the system consists of

- `rimokunControl`: operator-facing GUI
- `rimoServer`: server-side runtime and machine coordinator
- shared transport, configuration, and status models in `Utilities/`

The software coordinates remote manipulator motion, tool changing, vacuum clamp interaction, and subsystem monitoring. The server owns machine coordination and publishes status. The GUI connects to the server, sends commands, and renders current machine state for operators and engineers.

## Getting started

Build the project:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Run the server and GUI with a config file such as `Config/rimokun.yaml`:

```bash
./build/Server/apps/rimoServer --config Config/rimokun.yaml
./build/GUI/rimokunControl --config Config/rimokun.yaml
```

If your local build output uses different binary paths, adjust the commands accordingly.

## Documentation

Full documentation is intended to be published on GitHub Pages:

```text
https://piotrpodlaski.github.io/rimokun/
```

The docs site covers:

- architecture
- installation
- quickstart
- configuration
- GUI behavior
- server runtime
- command/status interfaces
- operation
- troubleshooting
- development

For local documentation work:

```bash
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
mkdocs serve
```

To publish the docs from this repository with GitHub Actions:

1. Push the documentation changes to the `master` branch.
2. In the GitHub repository, open `Settings -> Pages`.
3. Set the Pages source to `GitHub Actions`.
4. Let the existing workflow in `.github/workflows/docs.yml` build and deploy the site.

The published site URL for this repository is:

```text
https://piotrpodlaski.github.io/rimokun/
```

No custom domain is configured.

## Where to go next

- See `docs/architecture.md` for the system model
- See `docs/configuration.md` for runtime configuration structure
- See `docs/operation.md` for day-to-day run guidance
- See `docs/interfaces.md` for command and status behavior
