# MJSXJ02HL application

[![Donate](https://img.shields.io/badge/donate-Yandex-red.svg)](https://money.yandex.ru/to/4100110221014297)

Application for Xiaomi Smart Camera Standard Edition (MJSXJ02HL) with RTSP and MQTT support.

## Build

1. Install Hi3518Ev300 [toolchain](https://dl.openipc.org/SDK/HiSilicon/Hi3516Ev200_16Ev300_18Ev300/Hi3516EV200R001C01SPC011/arm-himix100-linux.tgz):

```bash
tar -zxf arm-himix100-linux.tgz
sudo ./arm-himix100-linux.install
```

2. Copy the libraries from directory `/usr/app/lib` of the original firmware to directory `/opt/hisi-linux/x86-arm/arm-himix100-linux/target/usr/app`.

3. Clone the repository:

```bash
git clone https://github.com/avdeevsv91/mjsxj02hl_application
cd mjsxj02hl_application
```

4. Copy the additional dependencies to directory `/opt/hisi-linux/x86-arm/arm-himix100-linux/target/usr/app`:

```bash
sudo cp -r dependencies/. /opt/hisi-linux/x86-arm/arm-himix100-linux/target/usr/app
```

5. Build application:
```bash
make
```

## Usage

```bash
mjsxj02hl [<action> [options...]]
```

Running without arguments starts the main thread of the application.

***--config <filename>*** Specify the location of the configuration file for the main thread of application. Default path is `/usr/app/share/mjsxj02hl.conf`

***--factory-reset*** Reset device settings to default values. Attention: this action cannot be undone!

***--get-image <filename>*** Output the camera image to a file. Requires a running main thread of mjsxj02hl application.

***--help*** Display help message.
