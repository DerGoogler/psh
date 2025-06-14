# psh — Privileged Shell Wrapper for Termux

`psh` is a lightweight C++ utility designed to provide a controlled root-like shell environment on Termux for Android. It wraps around existing `su` binaries to launch commands or shells with privilege escalation, while managing environment variables, user switching, and system paths carefully.

## Overview

`psh` acts as a smart front-end for `su` (superuser) commands in Termux. It provides:

- Switching to root or another user with environment isolation.
- Customizable PATH manipulation (prepend or append system paths).
- Support for specifying the shell executable to use.
- Debug logging for troubleshooting.
- Compatibility with various Android setups including Magisk and classic `su`.
- Handling of environment variables to maintain Termux user context inside root shells.
- Prevention of direct root execution of Termux itself (done externally by install scripts).

## Usage

```bash
psh [options] [user]
```

### Options

- `-p, --syspre`
  Prepend Android system paths (`/system/bin`, etc.) to the PATH.

- `-a, --sysadd`
  Append Android system paths to the PATH.

- `-s, --shell <shell>`
  Specify an alternate shell executable to use instead of the default.

- `--version`
  Show `psh` version.

- `-h, --help`
  Show usage help.

### Examples

- Run root shell with default environment:

  ```bash
  psh
  ```

- Switch to a different user:

  ```bash
  psh someuser
  ```

- Run a command as root preserving environment:

  ```bash
  psh -E ls /root
  ```

- Specify shell explicitly:

  ```bash
  psh -s /system/bin/sh
  ```

## Debugging

Run with the `--dbg` option to enable debug logging to a file `psh_debug_<YYYYMMDD>` in the current directory:

```bash
psh --dbg ...
```

Debug logs include environment variables, architecture, Android version, device info, kernel, and command execution details.

## Installation on Termux

You can install `psh` easily on Termux using the provided install script, which downloads the appropriate binary for your device architecture, sets permissions, and configures ownership:

Run this command directly in Termux (curl or wget):

```bash
curl -fsSL https://psh.dergoogler.com/install.sh | sh
```

Or with wget:

```bash
wget -qO- https://psh.dergoogler.com/install.sh | sh
```

By default, it installs the latest stable release. You can specify a version like so:

```bash
VERSION=1.0.0 curl -fsSL https://psh.dergoogler.com/install.sh | sh
```

**Note:** The installer aborts if run as root inside Termux, ensuring safe installation.
