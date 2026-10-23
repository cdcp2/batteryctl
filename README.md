# batteryctl

`batteryctl` is a small command-line program for Linux. It reads battery data from sysfs.
It can also set a charge limit or request an MSI performance mode when the driver supports the operation.

> [!WARNING]
> This software is experimental. Check the driver and firmware support before you change a setting.
> Do not force an unknown EC configuration. A wrong configuration can cause hardware problems.
> A successful read-back does not confirm the physical behavior of the battery or cooling system.

## Functions

- Find one battery through the `type` attribute in `/sys/class/power_supply/`.
- Show the charge level and the charge state.
- Show the start and end charge limits, if the driver provides them.
- Show the battery's estimated capacity health.
- Show the charge cycle count, if the driver provides it.
- Request a charge end limit of 60%, 80%, or 100%.
- Request an MSI performance mode: `eco`, `comfort`, or `turbo`.
- Read a setting again after a write to check the reported value.

## Requirements

To build the program, install a C11 compiler and GNU Make.

On Arch Linux, run:

```sh
sudo pacman -S --needed base-devel
```

Linux must provide the power supply sysfs interface.
The status and health commands do not need the MSI EC driver.
The limit command needs a driver that provides `charge_control_end_threshold`.
The performance command needs a compatible `msi-ec` driver that provides `shift_mode`.

See the [msi-ec project](https://github.com/BeardOverflow/msi-ec) for driver information.
`batteryctl` does not install or load that driver.

## Build

Open the project directory. Run:

```sh
make
```

Make creates `build/bin/batteryctl`.

Check the program before you install it:

```sh
./build/bin/batteryctl status
./build/bin/batteryctl health
```

Remove all build files:

```sh
make clean
```

## Install

Install the executable:

```sh
sudo make install
```

The default path is `/usr/local/bin/batteryctl`.

Check the installed command:

```sh
command -v batteryctl
batteryctl status
```

Remove the executable:

```sh
sudo make uninstall
```

Installation does not add a service. It does not set a charge limit at startup.
The user must run a command to change a setting.

## Commands

| Command | Function |
| --- | --- |
| `batteryctl status` | Show battery data and available settings. |
| `batteryctl health` | Show the estimated battery health and cycle count. |
| `batteryctl limit 60` | Request a 60% charge end limit. |
| `batteryctl limit 80` | Request an 80% charge end limit. |
| `batteryctl limit 100` | Request a 100% charge end limit. |
| `batteryctl performance eco` | Request the MSI eco mode. |
| `batteryctl performance comfort` | Request the MSI comfort mode. |
| `batteryctl performance turbo` | Request the MSI turbo mode. |

Read-only commands usually work without root privileges.
Write operations usually need root privileges.

For example:

```sh
batteryctl status
batteryctl health
sudo batteryctl limit 80
sudo batteryctl performance comfort
```

> [!CAUTION]
> Before you run a write command, make sure that the driver supports your laptop and EC firmware.
> Do not use `turbo` before you check the cooling behavior of your laptop.
> `limit 100` requests a 100% end limit. It does not necessarily restore the original EC state.

The program accepts only the three specified charge limits.
It does not detect all limits that the hardware can use.
The performance modes also depend on the installed driver and laptop model.

## Battery health

Example output:

```text
Battery health
------------------------------
  Device               BAT1
  Design capacity      4500 mAh
  Current full         4002 mAh
  Health               88.9%
  Cycle count          Unavailable
```

The program uses the following calculation:

```text
health (%) = current full capacity / design capacity * 100
```

The program first tries `energy_full` and `energy_full_design`.
It shows these values in mWh.
If the energy data is not available, it tries `charge_full` and `charge_full_design`.
It shows these values in mAh.

The health value is an estimate of the remaining capacity.
It is not a complete battery health test.
The cycle count can be unavailable.

## How the program works

Linux provides battery data through `/sys/class/power_supply/`.
The program reads the `type` attribute to find a battery.
The current version requires exactly one battery.

The program uses these standard attributes when they are available:

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

For MSI performance modes, the program uses:

```text
/sys/devices/platform/msi-ec/shift_mode
```

The program writes text to a sysfs attribute.
It then reads the attribute to check the reported value.
This check does not prove that the hardware applied the setting.

## Source files

| File | Function |
| --- | --- |
| `main.c` | Read and check command-line arguments. |
| `battery.c` | Find the battery and run battery operations. |
| `battery.h` | Declare the public battery functions. |
| `sysfs.c` | Read and write sysfs attributes. |
| `sysfs.h` | Declare the sysfs functions and result codes. |
| `Makefile` | Build and install the program. |

Make puts object files in `build/obj/`.
It puts the executable in `build/bin/`.
Git ignores the `build/` directory.

## Tested configuration

The initial tests used this laptop:

- Model: MSI Thin A15 B7VE.
- Operating system: Arch Linux.
- Kernel during initial tests: 7.2.11-arch1-1.
- EC firmware: `16RKIMS1.112`.
- Community `msi-ec` driver with a manual configuration override for `16RKIMS1.111`.

This override does not make the firmware officially supported.
Do not use the override on another laptop without a separate compatibility check.
The program has not been validated on all MSI models.

## Troubleshooting

**A value shows `Unavailable`.**

Check that the driver provides the required sysfs attribute.

```sh
ls /sys/class/power_supply/
ls /sys/devices/platform/msi-ec/ 2>/dev/null
```

**A write fails with `Permission denied`.**

Check that the driver supports the operation.
Then use an authorized account to run the command.

**The program does not find a battery.**

Check the `type` files in `/sys/class/power_supply/`.
The program stops if it finds more than one battery.

**The health command does not show a cycle count.**

The battery driver might not provide the cycle count.
Do not estimate a cycle count from the health percentage.

## Limits

- The program supports one battery at a time.
- The program accepts only 60%, 80%, and 100% charge end limits.
- The program does not read a list of supported limits from the driver.
- The program does not run at startup.
- The project does not yet have automated tests.

## Contribute

Read [CONTRIBUTING.md](CONTRIBUTING.md) before you open a pull request.
Report the model, kernel version, driver version, and the command that failed.
Do not include private data in public issue reports.

## License

This project uses the MIT License. See [LICENSE](LICENSE).
