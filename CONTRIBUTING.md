# Contributing to OpenCircuitX

Thanks for taking a look. This file covers the basics - for the full architecture walkthrough,
build details, and a worked example of adding a new gate type, see the
[Contributing Guide](docs/contributing.html) on the documentation site.

## Building from source

See the [Building](README.md#building) section in the README for Windows (Visual Studio 2022),
Linux, and macOS instructions.

## Submitting changes

1. Fork the repository and create a feature branch: `git checkout -b feature/my-change`
2. Make your changes, following the existing code style (4-space indent, Allman braces, `m_` prefix for members).
3. Open a pull request against `main` with a clear description of what changed and why.

There's no automated test suite yet - manual testing on a Release x64 build is the current
standard. If you're touching the circuit canvas or simulation engine, verify your change with
at least one save/load cycle and one simulation run.

If you're touching anything platform-sensitive (file paths, process execution, wxWidgets UI
code), test on Linux too if you can, not just Windows. A real Linux pass found over a dozen
bugs in existing, previously-merged code - crashes, silent failures, and layout bugs - none of
which were visible on Windows because the two platforms genuinely behave differently in
several places (GTK's `wxStyledTextCtrl` doesn't short-circuit the same way MSW's does, sizers
compute differently, executables need different environment setup to find their own runtime
libraries). Don't assume "works on Windows" means "works."

## Reporting issues

Open a GitHub issue with steps to reproduce, what you expected, and what happened instead.
For crashes, include the OS/build configuration (Debug or Release, x64).
