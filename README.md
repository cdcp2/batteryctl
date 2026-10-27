# batteryctl

`batteryctl` is a small command-line program for Linux. It reads battery and fan data from sysfs.

It can also request charge limits, MSI performance modes, fan modes, and Cooler Boost when the installed driver supports these operations.

> [!WARNING]
> This software is experimental. Check driver and firmware compatibility before you change a setting.
>
> Do not force an unknown Embedded Controller (EC) configuration. An incorrect configuration can cause hardware problems.
>
> A successful read-back does not confirm that the hardware applied a setting correctly.

## Features

### Battery

- Find a battery through the `type` attribute in `/sys/class/power_supply/`.
- Show the battery charge level and charge state.
- Show the start and end charge limits when available.
- Show the estimated battery health.
- Show the charge cycle count when available.
- Request a charge end limit of 60%, 80%, or 100%.

### Performance

- Show the current MSI performance mode.
- Request `eco`, `comfort`, or `turbo` mode.

### Fan control

- Show the current fan mode.
- List the available fan modes.
- Show the reported CPU and GPU fan levels.
- Request `auto`, `silent`, or `advanced` fan mode when supported.
- Show the current Cooler Boost state.
- Request Cooler Boost `on` or `off`.

The program reads a setting again after each write to check the reported value.

## Requirements

To build the program, install a C11 compiler and GNU Make.

On Arch Linux, run:

```sh
sudo pacman -S --needed base-devel
```

Linux must provide the power supply sysfs interface.

The `status` and `health` commands do not require the MSI EC driver.

The `limit` command requires a battery driver that provides `charge_control_end_threshold`.

The MSI-specific commands require a compatible `msi-ec` driver and the corresponding sysfs attributes.

| Operation | Required sysfs attribute |
| --- | --- |
| Performance mode | `shift_mode` |
| Fan mode | `fan_mode` |
| Available fan modes | `available_fan_modes` |
| Cooler Boost | `cooler_boost` |
| CPU fan level | `cpu/realtime_fan_speed` |
| GPU fan level | `gpu/realtime_fan_speed` |

See the [msi-ec project](https://github.com/BeardOverflow/msi-ec) for driver information.

`batteryctl` does not install or load this driver.

## Build

Open the project directory and run:

```sh
make
```

Make creates the executable:

```text
build/bin/batteryctl
```

Check the program before installation:

```sh
./build/bin/batteryctl status
./build/bin/batteryctl health
./build/bin/batteryctl fan status
./build/bin/batteryctl fan list
```

Remove the build files:

```sh
make clean
```

## Install

Install the executable:

```sh
sudo make install
```

The default installation path is:

```text
/usr/local/bin/batteryctl
```

Check the installation:

```sh
command -v batteryctl
batteryctl status
```

Remove the executable:

```sh
sudo make uninstall
```

Installation does not create a system service.

The program does not run automatically at startup. It does not change any hardware setting unless the user runs a write command.

## Commands

### Battery commands

| Command | Function |
| --- | --- |
| `batteryctl status` | Show battery data and available settings. |
| `batteryctl health` | Show estimated battery health and cycle count. |
| `batteryctl limit 60` | Request a 60% charge end limit. |
| `batteryctl limit 80` | Request an 80% charge end limit. |
| `batteryctl limit 100` | Request a 100% charge end limit. |

Example:

```sh
batteryctl status
batteryctl health
sudo batteryctl limit 80
```

### Performance commands

| Command | Function |
| --- | --- |
| `batteryctl performance eco` | Request MSI eco mode. |
| `batteryctl performance comfort` | Request MSI comfort mode. |
| `batteryctl performance turbo` | Request MSI turbo mode. |

Example:

```sh
sudo batteryctl performance comfort
```

### Fan commands

| Command | Function |
| --- | --- |
| `batteryctl fan status` | Show fan mode, fan levels, and Cooler Boost state. |
| `batteryctl fan list` | Show available fan modes. |
| `batteryctl fan auto` | Request automatic fan control. |
| `batteryctl fan silent` | Request silent fan mode. |
| `batteryctl fan advanced` | Request advanced fan mode. |
| `batteryctl fan boost on` | Request Cooler Boost activation. |
| `batteryctl fan boost off` | Request Cooler Boost deactivation. |

Read-only commands usually work without root privileges.

Write commands usually require root privileges.

> [!CAUTION]
> Check that your laptop and EC firmware are compatible before using fan control commands.
>
> The presence of a sysfs attribute does not prove that the associated hardware operation is safe.
>
> Do not use `silent`, `advanced`, or Cooler Boost on an unverified EC configuration.

## Battery status

Example:

```text
Battery
------------------------------
  Device               BAT1
  Capacity             73%
  Status               Not charging

Charge protection
------------------------------
  Start threshold      50%
  End threshold        60%

Performance
------------------------------
  Current mode         turbo
```

The available information depends on the battery driver.

Some attributes may show `Unavailable`.

### Charge limits

The program accepts three charge end limits:

- `60`: request a 60% end threshold.
- `80`: request an 80% end threshold.
- `100`: request a 100% end threshold.

The driver determines how the requested threshold is applied.

Some drivers also change the start threshold.

> [!IMPORTANT]
> `batteryctl limit 100` requests a 100% end threshold. It does not necessarily restore the original EC configuration.
>
> On the tested MSI configuration, a 100% end threshold can correspond to a 90% start threshold.

The program does not detect every charge limit supported by the hardware.

## Battery health

Example output:

```text
Battery health
------------------------------
  Device               BAT1
  Design capacity      4500 mAh
  Current full         3915 mAh
  Health               87.0%
  Cycle count          Unavailable
```

The program uses this calculation:

```text
health (%) = current full capacity / design capacity * 100
```

The program first tries:

```text
energy_full
energy_full_design
```

It reports these values in mWh.

If energy values are unavailable, it tries:

```text
charge_full
charge_full_design
```

It reports these values in mAh.

Battery health is an estimate based on values reported by the battery controller.

The reported full capacity can change because of measurement conditions and battery controller estimates.

A small change does not necessarily mean that the battery lost that amount of physical capacity in one day.

Battery health is not a complete battery diagnostic test.

The cycle count may be unavailable.

## Fan status

Example output:

```text
Fan status
------------------------------
  Mode                 advanced
  CPU fan              0%
  GPU fan              0%
  Cooler Boost         off
```

The program reads the fan information from the `msi-ec` driver.

The fan values are driver-reported levels. They are not RPM measurements.

A reported value of `0%` does not always prove that the physical fan is stopped.

An unsupported firmware configuration, an inactive device, or an unavailable sensor can affect the reported value.

### Available fan modes

Run:

```sh
batteryctl fan list
```

Example output:

```text
Available fan modes
------------------------------
  auto
  silent
  advanced
```

The available modes depend on the driver and laptop model.

The program checks the available modes before requesting a fan mode change.

The current MSI driver interface provides the following modes on the tested laptop:

- `auto`: request automatic fan control.
- `silent`: request the silent fan profile.
- `advanced`: request the advanced fan profile.

The actual behavior depends on the EC firmware.

### Cooler Boost

Cooler Boost is an MSI cooling feature.

When supported, the driver provides:

```text
/sys/devices/platform/msi-ec/cooler_boost
```

The attribute uses two states:

```text
on
off
```

The corresponding commands are:

```sh
sudo batteryctl fan boost on
sudo batteryctl fan boost off
```

These commands are available in the program, but their physical behavior has not been fully validated on the tested EC firmware.

The program reads the attribute after a write and compares the reported state with the requested state.

This check does not prove that the fan speed changed.

Cooler Boost may increase noise and power consumption. Do not assume that every MSI laptop supports it.

## How the program works

### Battery discovery

Linux provides battery information through:

```text
/sys/class/power_supply/
```

The program reads the `type` attribute to identify batteries.

The current version requires exactly one detected battery.

It stops if no battery or multiple batteries are detected.

The program uses these attributes when available:

```text
capacity
status
charge_control_start_threshold
charge_control_end_threshold
energy_full
energy_full_design
charge_full
charge_full_design
cycle_count
```

### MSI EC interface

The program uses this directory:

```text
/sys/devices/platform/msi-ec/
```

It accesses the following attributes:

```text
shift_mode
available_fan_modes
fan_mode
cooler_boost
cpu/realtime_fan_speed
gpu/realtime_fan_speed
```

The driver provides these interfaces. The program does not access EC registers directly.

### Read and write operations

The program uses sysfs to read device information and request changes.

For a write operation, it:

1. Validates the requested value.
2. Writes the value to the appropriate sysfs attribute.
3. Reads the attribute again.
4. Compares the reported value with the requested value.
5. Returns a success or failure code.

A successful read-back only confirms that the driver reports the requested value.

It does not prove that the hardware applied the setting correctly.

## Source files

| File | Function |
| --- | --- |
| `main.c` | Parse and validate command-line arguments. |
| `battery.c` | Find the battery and perform battery operations. |
| `battery.h` | Declare battery functions. |
| `fan.c` | Read fan information and request fan control operations. |
| `fan.h` | Declare fan functions. |
| `sysfs.c` | Read and write sysfs attributes and provide shared output helpers. |
| `sysfs.h` | Declare sysfs functions, output helpers, and result codes. |
| `Makefile` | Build, install, and uninstall the program. |

Make stores object files in:

```text
build/obj/
```

It stores the executable in:

```text
build/bin/
```

Git ignores the `build/` directory.

## Tested configuration

Initial development and tests used:

| Component | Configuration |
| --- | --- |
| Laptop | MSI Thin A15 B7VE |
| Operating system | Arch Linux |
| Kernel during initial tests | `7.2.11-arch1-1` |
| EC firmware | `16RKIMS1.112` |
| Driver | Community `msi-ec` driver |
| Driver configuration | Manual firmware override using `16RKIMS1.111` |

The override selects a driver configuration intended for another firmware version.

**This does not make `16RKIMS1.112` officially supported.**

The following operations have been observed on the tested configuration:

- Battery status and health reads.
- Charge threshold writes and read-back.
- Performance mode writes and read-back.
- Fan mode and available mode reads.
- Cooler Boost state reads.
- Rejection of invalid Cooler Boost arguments.

Fan speed reporting and the physical effects of fan control writes have not been fully validated.

The program has not been tested on all MSI laptops.

Do not copy the firmware override to another laptop without an independent compatibility check.

## Troubleshooting

### An attribute shows `Unavailable`

Check that the driver provides the required sysfs attribute:

```sh
ls /sys/class/power_supply/
ls /sys/devices/platform/msi-ec/ 2>/dev/null
```

Some attributes are optional.

An unavailable attribute does not always indicate a program error.

### A write fails with `Permission denied`

Check that the driver supports the operation.

Use an authorized account to run the command.

For example:

```sh
sudo batteryctl limit 80
```

Do not change sysfs file permissions to bypass the driver's access controls.

### The program does not find a battery

Check the `type` attributes under:

```text
/sys/class/power_supply/
```

The current version supports only one battery.

### The health command does not show a cycle count

The battery driver may not provide `cycle_count`.

Do not estimate the cycle count from the health percentage.

### Fan levels always show 0%

Check the driver values directly:

```sh
cat /sys/devices/platform/msi-ec/cpu/realtime_fan_speed
cat /sys/devices/platform/msi-ec/gpu/realtime_fan_speed
```

Compare the readings with the physical fan behavior.

If a fan is clearly running while the driver reports `0`, the reported value may be unreliable.

Check firmware compatibility before using any fan write command.

### Cooler Boost does not work

Check that the attribute exists:

```sh
ls -l /sys/devices/platform/msi-ec/cooler_boost
```

Read the current state:

```sh
cat /sys/devices/platform/msi-ec/cooler_boost
```

The presence of the attribute does not guarantee correct physical behavior.

Do not force EC register writes to bypass an unsupported operation.

## Limitations

- The program supports one battery at a time.
- The program accepts only 60%, 80%, and 100% charge end limits.
- The program does not detect all supported charge limits.
- MSI functions require compatible driver interfaces.
- Fan level readings depend on the driver and firmware.
- Fan control does not provide direct custom fan curve editing.
- The program does not validate the physical effect of every write.
- The program does not run automatically at startup.
- The program does not install or configure the MSI EC driver.
- The project does not yet have automated tests.

## Contributing

Contributions and bug reports are welcome.

Read [CONTRIBUTING.md](CONTRIBUTING.md) before opening a pull request.

When reporting a problem, include:

- Laptop model.
- Kernel version.
- EC firmware version.
- Driver version.
- Command that failed.
- Relevant error messages.

Do not include passwords, access tokens, private information, or other sensitive data in public reports.

## License

This project uses the MIT License.

See [LICENSE](LICENSE).
