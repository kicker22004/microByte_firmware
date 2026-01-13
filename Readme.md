![](images/microByte_logo.png)

MicroByte is a hand-held, open-source retro console powered by the ESP32-WROVER. The firmware included in this repository is focused on achieving a modular architecture to simplify usage and to allow easy extension and modification. This is a modified version to the original software from JFM92. 

(**If you are looking for general information about this project, please visit: [GitHub microByte](https://github.com/jfm92/microByte)**)

The basic architecture of the software is shown in the following diagram:

// add diagram

If you want to test or modify this software, follow the steps below.

# Development Environment

This software is developed using **ESP-IDF v4.4**, the official development framework for Espressif microcontrollers.

You may also use the setup described in the official Espressif guides; however, deviations from the configuration below may cause build or compatibility issues.

- [ESP-IDF v4.4 Get Started Guide](https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/get-started/)

## Requirements

### Hardware

- microByte console
- USB Type-C to Type-A cable
- PC running Windows or Linux (macOS may work but is not officially tested)

### Software

#### Linux

- Ubuntu 18.04 or 20.04
- Visual Studio Code

#### Windows

- Windows 10 version 1903 or later
- Visual Studio Code
- WSL1 (recommended for USB serial stability)
- Ubuntu 18.04 LTS for WSL
- Windows Terminal

## Installing ESP-IDF

When using WSL on Windows, Linux runs natively inside Windows. Therefore, the same dependencies apply to both operating systems.

Open a terminal (Windows Terminal when using WSL) and install the required dependencies:

```console
sudo apt-get update
sudo apt-get install -y git wget flex bison gperf python3 python3-pip python3-setuptools cmake ninja-build ccache libffi-dev libssl-dev dfu-util
```

### Create Workspace

#### Windows (WSL)

To allow easy access from the Windows file system, it is recommended to place the workspace inside the mounted Windows directory:

```console
mkdir /mnt/c/Users/<user_name>/Documents/esp-ws
cd /mnt/c/Users/<user_name>/Documents/esp-ws
```

This allows access to the workspace directly from the Windows *Documents* folder.

#### Linux

```console
mkdir ~/esp-ws
cd ~/esp-ws
```

### Download ESP-IDF v4.4

```console
git clone https://github.com/espressif/esp-idf -b v4.4
```

### Install ESP-IDF

```console
cd esp-idf
./install.sh
. ./export.sh
```

> **Note:** Each time you open a new terminal, you must re-source the ESP-IDF environment using:
>
> ```bash
> . ./export.sh
> ```
>
> This does **not** reinstall ESP-IDF; it only reloads the required environment variables.

## Build

Using the same terminal session, execute the following commands. The process is identical on Windows (WSL) and Linux.

Clone the firmware repository including submodules:

```console
git clone https://github.com/kicker22004/microByte_firmware --recursive
```

Build the project:

```console
cd microByte_firmware
make -j4
```

The build process may take a few minutes. If successful, the firmware binaries will be generated and ready to flash.

Do not close this terminal, as the configured ESP-IDF environment will be reused in the next steps.

## Flashing

Before flashing the device:
- Connect the USB cable
- Power on the microByte device
- Identify the assigned serial port

### Windows (WSL)

Open **Device Manager** and identify the COM port assigned to the device (for example, `COM4`).

In WSL, Windows COM ports are mapped as:

- `COM4` → `/dev/ttyS4`

Start the flashing process:

```console
make -j12 flash ESPPORT=/dev/ttyS4 ESPBAUD=2000000
```

- **ESPPORT**: Serial port connected to the device
- **ESPBAUD**: Flash baud rate (default is 2 Mbit)

If flashing is successful, the device will automatically reset and initialize.

### Linux

Identify the serial device:

```console
dmesg
```

Look for the `ch341-uart` device, for example:

```text
ttyUSB0
```

Flash the device:

```console
make -j12 flash ESPPORT=/dev/ttyUSB0 ESPBAUD=2000000
```

If successful, the device will reset and boot the firmware.

## Debug Monitor

The microByte firmware uses the ESP-IDF logging system, providing detailed runtime diagnostics useful during development.

### Steps

1. Ensure ESP-IDF is installed and the environment is sourced
2. Identify the serial port used by the device
3. Run the monitor command:

```console
make -j12 monitor ESPPORT=<serial-port> ESPBAUD=2000000
```

This will display boot logs, system initialization, and runtime messages.

To exit the monitor, press:

```text
Ctrl + ]
```

During gameplay, emulator frame rate information is periodically printed to the log output.

