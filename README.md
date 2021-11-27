# MJSXJ02HL application

[![Donate](https://img.shields.io/badge/donate-Yandex-red.svg)](https://money.yandex.ru/to/4100110221014297)

Application for Xiaomi Smart Camera Standard Edition (MJSXJ02HL) with RTSP and MQTT support.

## Build

1. Install Hi3518Ev300 [toolchain](https://dl.openipc.org/SDK/HiSilicon/Hi3516Ev200_16Ev300_18Ev300/Hi3516EV200R001C01SPC011/arm-himix100-linux.tgz):

```bash
tar -zxf arm-himix100-linux.tgz
sudo ./arm-himix100-linux.install
```

2. Copy the libraries from directory `/usr/app/lib` of the original firmware to directory `/opt/hisi-linux/x86-arm/arm-himix100-linux/target/usr/app/lib`.

3. Clone the repository:

```bash
git clone https://github.com/kasitoru/mjsxj02hl_application
cd mjsxj02hl_application
```

4. Changing the permissions for the folder `/opt/hisi-linux/x86-arm/arm-himix100-linux/target/usr/app/lib`:

```bash
sudo chmod 755 /opt/hisi-linux/x86-arm/arm-himix100-linux/target/usr/app/lib
```

5. Update submodules (optional):
```bash
make update-libs
```

6. Build application:
```bash
make
```

To save time, you can disable the build of external libraries:

```bash
make SKIP_EXTERNAL_LIBS=ON
```

...and skip step №5.

## Configuration

Default config `/usr/app/share/mjsxj02hl.conf`:

```ini
[general]
led = true                     ; Enable onboard LED indicator

[logger]
level = 2                      ; Log level (0 = disable, 1 = error, 2 = warning, 3 = info, 4 = debug)
file =                         ; Write log to file (empty for disable)

[osd]
enable = false                 ; Enable On-Screen Display (OSD)
oemlogo = true                 ; Display OEM logo (MI)
oemlogo_x = 2                  ; X position of the OEM logo
oemlogo_y = 0                  ; Y position of the OEM logo
oemlogo_size = 0               ; Size of the OEM logo (can take negative values)
datetime = true                ; Display date and time
datetime_x = 48                ; X position of the date and time
datetime_y = 0                 ; Y position of the date and time
datetime_size = 0              ; Size of the date and time (can take negative values)
motion = false                 ; Display detected motions in rectangles
humanoid = false               ; Display detected humanoids in rectangles

[video]
gop = 1                        ; Group of pictures (GOP) every N*FPS (20)
flip = false                   ; Flip image (all channels)
mirror = false                 ; Mirror image (all channels)
primary_enable = true          ; Enable video for primary channel
secondary_enable = true        ; Enable video for secondary channel
primary_type = 1               ; Video compression standard for primary channel (1 = h264, 2 = h265)
secondary_type = 1             ; Video compression standard for secondary channel (1 = h264, 2 = h265)
primary_bitrate = 1800         ; Bitrate for primary channel
secondary_bitrate = 900        ; Bitrate for secondary channel
primary_rcmode = 2             ; Rate control mode for primary channel (0 = constant bitrate, 1 = constant quality, 2 = variable bitrate)
secondary_rcmode = 2           ; Rate control mode for secondary channel (0 = constant bitrate, 1 = constant quality, 2 = variable bitrate)

[audio]
volume = 70                    ; Audio volume level (0-100)
primary_enable = true          ; Enable audio for primary channel
secondary_enable = true        ; Enable audio for secondary channel

[speaker]
volume = 70                    ; Speaker volume level (0-100)
type = 1                       ; Default file format (1 = PCM, 2 = G711)

[alarm]
motion_sens = 150              ; Motion sensitivity (1-255)
humanoid_sens = 150            ; Humanoid sensitivity (1-255)
motion_timeout = 60            ; Motion timeout (in seconds)
humanoid_timeout = 60          ; Humanoid timeout (in seconds)
motion_detect_exec =           ; Execute the command when motion is detected (empty for disable)
humanoid_detect_exec =         ; Execute the command when humanoid is detected (empty for disable)
motion_lost_exec =             ; Execute the command when motion is lost (empty for disable)
humanoid_lost_exec =           ; Execute the command when humanoid is lost (empty for disable)

[rtsp]
enable = true                  ; Enable RTSP server
port = 554                     ; Port number
username =                     ; Username (empty for disable)
password =                     ; Password
primary_name = primary         ; Name of the primary channel
secondary_name = secondary     ; Name of the secondary channel
primary_multicast = false      ; Use multicast for primary channel
secondary_multicast = false    ; Use multicast for secondary channel
primary_split_vframes = true   ; Split video frames into separate packets for primary channel
secondary_split_vframes = true ; Split video frames into separate packets for secondary channel

[mqtt]
enable = false                 ; Enable MQTT client
server =                       ; Server address
port = 1883                    ; Port number
username =                     ; Username (empty for anonimous)
password =                     ; Password (empty for disable)
topic = mjsxj02hl              ; Topic name
qos = 1                        ; Quality of Service (0, 1 or 2)
retain = false                 ; Retained messages
reconnection_interval = 60     ; Reconnection interval (in seconds)

[night]
mode = 2                       ; Night mode (0 = off, 1 = on, 2 = auto)
gray = 2                       ; Grayscale (0 = off, 1 = on, 2 = auto)
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

Network URL: `rtsp://<ip-address>:<port>/<channel_name>`

Example: `rtsp://192.168.1.18:554/primary`

## MQTT

### Input topics

**Topic: mjsxj02hl/cmd**

Execute the specified command on the device.

Command | Parameters | Description | Example payload
------- | ---------- | ----------- | ---------------
`get_image` | `filename` (string) | Save the image to the specified file (JPEG, 640x360). | { "action": "get_image", "filename": "/mnt/mmc/image.jpg" }
`set_volume` | `value` (integer) | Set volume level for speaker (0-100). | { "action": "set_volume", "value": 100 }
`play_media` | `filename` (string), `type` (string, optional), `volume` (integer, optional) | Play the specified media file. Two types are supported: "pcm" (WAV, 8000 hz, 16-bit, mono) and "g711" (A-Law, 8000 hz, 16-bit, mono). | { "action": "play_media", "filename": "/mnt/mmc/media.wav", "type": "pcm", "volume": 75 }
`stop_media` | | Stop current playback. | { "action": "stop_media" }
`restart` | | Restart the main thread of mjsxj02hl application | { "action": "restart" }
`reboot` | | Reboot the device. | { "action": "reboot" }

### Output topics

**Topic: mjsxj02hl/info**

This is a topic where general device state is published.

Field | Description
----- | -----------
`sdk_version` | Version of localsdk library.
`fw_version` | Version of the firmware.
`startup` | Application startup timestamp.
`timestamp` | Current timestamp.
`ip_address` | IP address of the device.
`total_ram` | The total size of RAM.
`free_ram` | The size of the free RAM.
`total_sdmem` | The total size of SD-card.
`free_sdmem` | The size of free space on the SD-card.
`total_configs` | Total size of the configs partition.
`free_configs` | The size of free space on the configs partition.
`volume_level` | Current volume level of the speaker.
`media_status` | Playback status (0 = stopped, 1 = playing, 2 = stopping).
`image_url` | URL address of the JPEG image from the camera.

**Topic: mjsxj02hl/alarm**

This is a topic where motion detection events is published.

Field | Description
----- | -----------
`motion` | Motion detection state.
`humanoid` | Humanoid detection state.
`timestamp` | Current timestamp.

**Topic: mjsxj02hl/night**

This is a topic where the state of night mode is published.

Field | Description
----- | -----------
`state` | Night mode state.
`gray` | Grayscale state.
`timestamp` | Current timestamp.

## Third-party libraries

* yyjson: https://github.com/ibireme/yyjson
* inih: https://github.com/benhoyt/inih
* paho.mqtt.c: https://github.com/eclipse/paho.mqtt.c
* RtspServer: https://github.com/PHZ76/RtspServer
