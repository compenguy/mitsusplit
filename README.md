# mitsusplit

Rust project to automate a mitubishi heat pump through its serial interface.

User configuration occurs through an http web interface or via an MQTT broker.

## Build Setup

This project uses an ESP32 SoC which is not, as of this writing, fully
supported by the rust toolchain, and additionally requires compiler features
only available in the `nightly` toolchain.

### Rust Nightly

With rustup and rust installed, enable the nightly toolchain with the following
commands:

```bash
$ rustup toolchain install nightly
$ rustup default nightly
```

### Target Architecture Support

Next the necessary compiler targets must be enabled.

For RISC-V based ESP32 targets (ESP32-C3) there is official compiler support,
so all that's necessary is the following:

```bash
$ rustup target add riscv32imc-unknown-none-elf
```

For Xtensa architecture targets, a special fork of the rust compiler is
required. Please consult the
[Rust on ESP Book](https://esp-rs.github.io/book/dependencies/installing-rust.html).

### Linker Support

ldproxy is a simple tool to forward linker arguments given to ldproxy to the actual linker executable.

To install:

```bash
$ cargo install ldproxy
```

For additional information, please consult the
[Rust on ESP Book](https://esp-rs.github.io/book/dependencies/build-tools.html).

### Flash and Debugging Support

#### espflash

`espflash` is a serial flasher utility for ESP devices. It can be installed as
a cargo command as follows:

```bash
$ cargo install cargo-espflash
$ cargo espflash --example=blinky --release --monitor
```

#### espmonitor

`espmonitor` is a debugging utility and can be installed and used in a similar
manner:

```bash
$ cargo install cargo-espmonitor
```

#### probe-rs

`[probe-rs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/api-guides/jtag-debugging/configure-builtin-jtag.html)`
also supports programming and debugging. If developing from linux, some additional udev rules may be required:

```
# Espressif dev kit FTDI
ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6010", MODE="660", GROUP="plugdev", TAG+="uaccess"

# Espressif USB JTAG/serial debug unit
ATTRS{idVendor}=="303a", ATTRS{idProduct}=="1001", MODE="660", GROUP="plugdev", TAG+="uaccess"

# Espressif USB Bridge
ATTRS{idVendor}=="303a", ATTRS{idProduct}=="1002", MODE="660", GROUP="plugdev", TAG+="uaccess"
```

#### OpenOCD

For RISC-V-based ESP32 targets (ESP32-C3), openocd works out of the box:

```bash
$ openocd -f board/esp32c3-builtin.cfg
```

Other architectures will likely require [Espressif's fork](https://github.com/espressif/openocd-esp32),
and the debugging interface will need to be specified separately from the target:

```bash
$ openocd -f interface/jlink.cfg -f target/esp32.cfg
```

## Build

```bash
$ cargo build
```

## Flash

```bash
$ echo "TODO"
```
