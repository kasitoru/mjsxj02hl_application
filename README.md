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

## Configuration

Default config `/usr/app/share/mjsxj02hl.conf`:

```ini
[logger]
level = 2              ; Log level (0 = disable, 1 = error, 2 = warning, 3 = info, 4 = debug)
file =                 ; Write log to file (empty for disable)

[video]
flip = false           ; Flip image
mirror = false         ; Mirror image

[audio]
volume = 70            ; Audio volume level (0-100)

[speaker]
volume = 70            ; Speaker volume level (0-100)

[alarm]
motion_sens = 150      ; Motion sensitivity (1-255)
humanoid_sens = 150    ; Humanoid sensitivity (1-255)
motion_timeout = 60    ; Motion timeout (in seconds)
humanoid_timeout = 60  ; Humanoid timeout (in seconds)
motion_detect_exec =   ; Execute the command when motion is detected (empty for disable)
humanoid_detect_exec = ; Execute the command when humanoid is detected (empty for disable)
motion_lost_exec =     ; Execute the command when motion is lost (empty for disable)
humanoid_lost_exec =   ; Execute the command when humanoid is lost (empty for disable)

[mqtt]
server =               ; Address (empty for disable)
port = 1883            ; Port
username =             ; Username (empty for anonimous)
password =             ; Password (empty for disable)
topic = mjsxj02hl      ; Topic name
qos = 1                ; Quality of Service (0, 1 or 2)
retain = false         ; Retained messages

[night]
mode = 2               ; Night mode (0 = off, 1 = on, 2 = auto)
gray = 2               ; Grayscale (0 = off, 1 = on, 2 = auto)
```

## Usage

```bash
mjsxj02hl [<action> [options...]]
```

Running without arguments starts the main thread of the application.

***--config <filename>*** Specify the location of the configuration file for the main thread of application.

***--factory-reset*** Reset device settings to default values. Attention: this action cannot be undone!

***--get-image <filename>*** Output the camera image to a file. Requires a running main thread of mjsxj02hl application.

***--help*** Display help message.

## RTSP

Network URL: `rtsp://<ip-address>:554`

## MQTT

### Input topics

**Topic: mjsxj02hl/cmd**

Execute the specified command on the device.

Command | Parameters | Description | Example payload
------- | ---------- | ----------- | ---------------
`get_image` | `filename` (string) | Save the image to the specified file (JPEG, 640x360). | { "action": "get_image", "filename": "/mnt/mmc/image.jpg" }
`set_volume` | `value` (integer) | Set volume level for speaker (0-100). | { "action": "set_volume", "value": 100 }
`play_media` | `filename` (string) | Play the specified media file (WAV, 8000 hz, 16-bit, mono). | { "action": "play_media", "filename": "/mnt/mmc/media.wav" }
