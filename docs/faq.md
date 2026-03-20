# FAQ

## Is this project an API library?

No. The repository is structured as a complete system with a GUI application, a runtime server, and command/status communication between them.

## What should I run first?

Start the server first, then launch the GUI. That gives the GUI a live command and status endpoint to connect to.

## What are the main executables?

The current repository indicates two primary executables:

- `rimoServer`
- `rimokunControl`

## Does the GUI talk directly to hardware?

The architecture suggests that machine-facing control is server-owned. The GUI acts as the operator client and uses the command/status interfaces to interact with the server.

## Where is configuration stored?

The repository includes `Config/rimokun.yaml` as the current example configuration. Both GUI and server accept a `--config` option.

## How do GUI and server communicate?

The shared transport code indicates a ZeroMQ-based design with separate channels for command/request-response traffic and status publication.

## Can I treat the status stream as command acknowledgement?

Not by itself. Command responses and status updates serve different purposes. Use the command response to understand request outcome and the status stream to understand ongoing machine state.

## What should I check first if the GUI cannot connect?

Confirm:

- the server is running
- both processes use compatible config files
- the configured command and status addresses match

## How do I build the docs locally?

Install `requirements.txt` and run:

```bash
mkdocs serve
```

## Where will the documentation site be published?

When GitHub Pages is enabled for the repository, the published URL will typically be:

```text
https://<owner>.github.io/<repository>/
```

Replace `<owner>` and `<repository>` with the actual GitHub account and repository names.
