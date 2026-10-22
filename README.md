# batteryctl

A small command-line tool written in C to inspect battery information and configure supported battery charge limits and MSI performance modes on Linux.

`batteryctl` uses the Linux **sysfs** interface. It does not communicate with the embedded controller (EC) directly, and it does not install, load, or modify kernel drivers.

> [!WARNING]
> Charge-limit and performance-mode changes depend on your kernel driver and hardware. Unsupported EC firmware configurations can be unsafe. **Do not force a firmware configuration or enable a performance mode merely because this tool accepts the command.** Verify hardware and driver compatibility first.

## Features

- Automatically find a battery using `/sys/class/power_supply/*/type` rather than assuming `BAT0` or `BAT1`.
- Display battery charge percentage, charging status, charge thresholds, and an MSI performance mode when available.
- Estimate battery capacity health from `energy_full` / `energy_full_design`, falling back to `charge_full` / `charge_full_design`.
- Display a battery cycle count if the driver provides one.
- Set a charge-end threshold to **60%, 80%, or 100%**, with read-back verification.
- Request an MSI performance mode: `eco`, `comfort`, or `turbo`, with read-back verification.
- Report missing optional sysfs attributes as unavailable.

## Requirements

- Linux with the `power_supply` sysfs interface mounted at `/sys/class/power_supply`.
- A C11 compiler and GNU Make to build the program.
- A driver exposing `charge_control_end_threshold` to change charging thresholds.
- A compatible `msi-ec` driver exposing `/sys/devices/platform/msi-ec/shift_mode` to change MSI performance modes.

Read-only commands may work without `msi-ec`. Features depend on what the particular driver exports. See the [msi-ec project](https://github.com/BeardOverflow/msi-ec) for its supported firmware, setup instructions, and warnings. Installing `batteryctl` does **not** install `msi-ec`.

On Arch Linux, install the build tools if necessary:

```bash
sudo pacman -S --needed base-devel
```

## Build

From the project directory:

```bash
make
```

The executable is created at:

```text
build/bin/batteryctl
```

Try the read-only commands before installation:

```bash
./build/bin/batteryctl status
./build/bin/batteryctl health
```

Remove generated files:

```bash
make clean
```

## Install and uninstall

```bash
sudo make install
```

The default installation path is `/usr/local/bin/batteryctl`. Check it with:

```bash
command -v batteryctl
batteryctl status
```

To remove the installed executable:

```bash
sudo make uninstall
```

Installation does **not** enable a systemd service or apply a charge limit on startup. Commands run only when requested. Loading the required kernel driver at boot is a separate system configuration task.

## Commands

| Command | Description | Typical permissions |
| --- | --- | --- |
| `batteryctl status` | Show charge percentage, status, available charge thresholds, and current MSI performance mode. | Regular user |
| `batteryctl health` | Show design capacity, estimated current full capacity, capacity retention, and cycle count if available. | Regular user |
| `batteryctl limit 60` | Request a 60% charge-end threshold. | Root / write access |
| `batteryctl limit 80` | Request an 80% charge-end threshold. | Root / write access |
| `batteryctl limit 100` | Request a 100% charge-end threshold. | Root / write access |
| `batteryctl performance eco` | Request the MSI power-saving mode. | Root / write access |
| `batteryctl performance comfort` | Request the MSI balanced mode. | Root / write access |
| `batteryctl performance turbo` | Request the MSI high-performance mode. | Root / write access |

For example:

```bash
batteryctl status
batteryctl health
sudo batteryctl limit 80
sudo batteryctl performance comfort
```

**Important:** `limit 100` requests an upper threshold of 100%; it is **not** guaranteed to restore the EC's original or unconfigured state. On some MSI configurations, the corresponding start threshold is 10 percentage points below the end threshold (for example, 70% / 80%). Do not assume this relationship applies to every driver.

`performance` currently accepts a fixed set of mode names. Their availability and effects depend on the loaded driver and model; the tool does not yet discover all supported shift modes dynamically. In particular, `turbo` can affect power use and thermals.

## Battery health

Example output (illustrative):

```text
Battery health
------------------------------
  Device               BAT1
  Design capacity      4500 mAh
  Current full         4002 mAh
  Health               88.9%
  Cycle count          Unavailable
```

The calculation is:

```text
health (%) = current full capacity / design capacity * 100
```

- When available, `energy_*` values are converted from **µWh** to **mWh**.
- Otherwise, `charge_*` values are converted from **µAh** to **mAh**.
- `cycle_count` is optional; `Unavailable` means the driver does not provide a usable value.
- Health is an **estimate of retained capacity**, not a complete battery diagnostic.

## How it works

The program reads standard Linux `power_supply` attributes from a battery it discovers by checking each device's `type` attribute:

```text
/sys/class/power_supply/<battery>/capacity
/sys/class/power_supply/<battery>/status
/sys/class/power_supply/<battery>/charge_control_start_threshold
/sys/class/power_supply/<battery>/charge_control_end_threshold
/sys/class/power_supply/<battery>/energy_full
/sys/class/power_supply/<battery>/energy_full_design
/sys/class/power_supply/<battery>/charge_full
/sys/class/power_supply/<battery>/charge_full_design
/sys/class/power_supply/<battery>/cycle_count
```

MSI performance-mode commands use:

```text
/sys/devices/platform/msi-ec/shift_mode
```

The program checks write results and reads an attribute again to compare the reported value with the requested setting. **Read-back verifies what the driver reports, not necessarily the physical charging or thermal behavior.**

The current implementation requires exactly one discoverable battery. If multiple batteries are found, it returns an error instead of selecting one arbitrarily.

## Source layout

```text
.
├── main.c       # CLI argument handling
├── battery.c    # Battery discovery, status, health, and control
├── battery.h    # Public battery operations
├── sysfs.c      # sysfs file reads and writes
├── sysfs.h      # Shared I/O results and declarations
├── Makefile
├── README.md
└── build/        # Generated object files and executable (ignored by Git)
    ├── obj/
    └── bin/
```

## Troubleshooting

**`Unavailable` appears for thresholds or performance mode:** The loaded driver may not export those attributes. Check what is present:

```bash
ls /sys/class/power_supply/
ls /sys/devices/platform/msi-ec/ 2>/dev/null
```

**Write fails with `Permission denied`:** sysfs controls commonly require elevated privileges. Confirm the device/driver is compatible before retrying with `sudo`.

**Write fails despite root permissions:** The driver may not support the attribute or selected value. Check kernel logs and the driver's documentation; do not attempt raw EC register writes as a workaround.

**Battery health data is missing:** A driver may export `charge_*` but not `energy_*`, or vice versa. Some batteries do not expose `cycle_count`.

## Current limitations and planned improvements

- Only one battery is supported at a time.
- Allowed charge limits are currently fixed at 60%, 80%, and 100%.
- MSI performance modes are validated against a fixed list, not a dynamically queried capability list.
- No automatic service, scheduled policy, or background daemon is provided.
- Automated tests and broader hardware validation are future work.

## Safety and compatibility

This project is under development and was initially built around an MSI laptop running Linux. **It is not a guarantee of compatibility with every MSI device or other vendors.** Verify kernel/driver support and observe the actual behavior before relying on charging or performance controls.

Do not use an unsupported EC firmware override simply to make sysfs attributes appear. Follow the upstream driver's guidance, and keep a way to revert driver changes.

## Contributing

Bug reports and patches are welcome. Include the operating system, kernel version, laptop model, driver version, and the exact command/output where useful. Do **not** post passwords, authentication tokens, private identifiers, or raw firmware data without reviewing them first.

## License

A license has not yet been selected. Add a `LICENSE` file before inviting code reuse or contributions under an open-source license.
