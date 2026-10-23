# Contribute to batteryctl

Thank you for your interest in this project.

## Before you start

1. Read `README.md`.
2. Check the open issues.
3. Make one change at a time.
4. Do not add code that writes directly to EC registers.

## Build the program

```sh
make clean
make
```

Make sure that the compiler shows no warnings.

## Check a change

Run the read-only commands first:

```sh
./build/bin/batteryctl status
./build/bin/batteryctl health
```

Do not run write commands on untested hardware.
Use temporary files for tests of the sysfs functions.
Do not use real EC registers as test inputs.

## Report a problem

Include this information in the issue:

- Laptop manufacturer and model.
- Operating system and kernel version.
- Driver name and version.
- EC firmware version, if you know it.
- Command that failed.
- Error message or relevant log lines.
- Steps to reproduce the problem.

Remove passwords, tokens, serial numbers, and other private data.
Do not publish a raw firmware dump.

## Submit a change

1. Create a branch for the change.
2. Make the change.
3. Build the program.
4. Run the available tests.
5. Write a commit message that describes the change.
6. Open a pull request.

Explain the purpose of the change.
Describe how you checked it.
State if you did not test the change on hardware.

## Document a change

Use clear technical English. Follow these ASD-STE100 writing principles:

- Use short sentences.
- Use the active voice.
- Write one instruction in each sentence.
- Use the same term for the same component.
- Explain each command before you give the command.

The project uses these principles as guidance. It does not claim formal ASD-STE100 compliance.
