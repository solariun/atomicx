# Arduino procedures

## Install arduino-cli

## Useful commands

```
arduino-cli board details -b arduino:avr:uno --list-programmers

arduino-cli board details "$1"

arduino-cli board listall

arduino-cli board listall esp8266:esp8266:wifi

arduino-cli board listall avr

```

## Compiling and uploading

```bash

arduino-cli compile -b arduino:avr:nano:cpu=atmega328  --upload -p "$1" "$2"

# upload AVR using USB-ASP
arduino-cli compile -b arduino:avr:nano:cpu=atmega328  --upload -P usbasp  "$1"

# upload using avrisp XSP
arduino-cli compile -b arduino:avr:nano:cpu=atmega328  --upload -P avrisp -p "$1" "$2"

arduino-cli compile -u -p "$1"  -b esp32:esp32:heltec_wifi_kit_32  "$2"

arduino-cli compile -u  -p "$1" -b Arduino_STM32:STM32F1:genericSTM32F103C:upload_method=jlinkMethod,opt=o3std "$2"

arduino-cli compile -v -b esp8266:esp8266:wifi_kit_8:baud=921600 -upload -p "<serial>" "<arduino>"

arduino-cli compile -b arduino:mbed_nano:nano33ble -upload -p "<serial>" "<arduino>"

# Using ST-LINK
arduino-cli compile -v -b STMicroelectronics:stm32:GenF1:pnum=BLUEPILL_F103C8,usb=CDCgen -upload -p "." "<arduino>"

```

Despite the lack of feature parity at the moment, Arduino CLI provides many of the features you can find in the Arduino
IDE. Let's see some examples.

### LIST CPUS and other options from a board

````
arduino-cli board details -b STMicroelectronics:stm32:GenF1

arduino-cli board details -b arduino:avr:nano
````

## Before you start

`arduino-cli` is a container of commands and each command has its own dedicated help text that can be shown with the
`help` command like this:

```console
$ arduino-cli help core
Arduino Core operations.

Usage:
    arduino-cli core [command]

Examples:
    ./arduino-cli core update-index

Available Commands:
    download     Downloads one or more cores and corresponding tool dependencies.
    install      Installs one or more cores and corresponding tool dependencies.
    list         Shows the list of installed platforms.
    search       Search for a core in Boards Manager.
    uninstall    Uninstalls one or more cores and corresponding tool dependencies if no more used.
    update-index Updates the index of cores.
    upgrade      Upgrades one or all installed platforms to the latest version.

Flags:
    -h, --help   help for core

Global Flags:
        --additional-urls strings   Additional URLs for Boards Manager.
        --config-file string        The custom config file (if not specified the default will be used).
        --format string             The output format, can be [text|json]. (default "text")
        --log-file string           Path to the file where logs will be written.
        --log-format string         The output format for the logs, can be [text|json].
        --log-level string          Messages with this level and above will be logged.
    -v, --verbose                   Print the logs on the standard output.

Use "arduino-cli core [command] --help" for more information about a command.
```

## Create a configuration file

Arduino CLI doesn't strictly require a configuration file to work because the command line interface provides any
possible functionality. However, having one can spare you a lot of typing when issuing a command, so let's go ahead and
create it with:

```sh
$ arduino-cli config init
Config file written: /home/luca/.arduino15/arduino-cli.yaml
```

If you inspect the contents of `arduino-cli.yaml`, you'll find the available options with their respective default
values. For more information, see the [configuration documentation].

## Create a new sketch

To create a new sketch named `MyFirstSketch` in the current directory, run the following command:

```sh
$ arduino-cli sketch new MyFirstSketch
Sketch created in: /home/luca/MyFirstSketch
```

A sketch is a folder containing assets like source files and libraries; the `new` command creates for you a .ino file
called `MyFirstSketch.ino` containing Arduino boilerplate code:

```sh
$ cat $HOME/MyFirstSketch/MyFirstSketch.ino
void setup() {
}

void loop() {
}
```

At this point you can use your favourite file editor or IDE to open the file `$HOME/MyFirstSketch/MyFirstSketch.ino` and
change the code like this:

```c
void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
}
```

## Connect the board to your PC

The first thing to do upon a fresh install is to update the local cache of available platforms and libraries by running:

```sh
$ arduino-cli core update-index
Updating index: package_index.json downloaded
```

After connecting the board to your PC by using the USB cable, you should be able to check whether it's been recognized
by running:

```sh
$ arduino-cli board list
Port         Type              Board Name              FQBN                 Core
/dev/ttyACM1 Serial Port (USB) Arduino/Genuino MKR1000 arduino:samd:mkr1000 arduino:samd
```

In this example, the MKR1000 board was recognized and from the output of the command you see the platform core called
`arduino:samd` is the one that needs to be installed to make it work.

If you see an `Unknown` board listed, uploading should still work as long as you identify the platform core and use the
correct FQBN string. When a board is not detected for whatever reason, you can list all the supported boards and their
FQBN strings by running the following:

```sh
$ arduino-cli board listall mkr
Board Name              FQBN
Arduino MKR FOX 1200    arduino:samd:mkrfox1200
Arduino MKR GSM 1400    arduino:samd:mkrgsm1400
Arduino MKR WAN 1300    arduino:samd:mkrwan1300
Arduino MKR WiFi 1010   arduino:samd:mkrwifi1010
Arduino MKRZERO         arduino:samd:mkrzero
Arduino/Genuino MKR1000 arduino:samd:mkr1000
```

## Install the core for your board

To install the `arduino:samd` platform core, run the following:

```sh
$ arduino-cli core install arduino:samd
Downloading tools...
arduino:arm-none-eabi-gcc@4.8.3-2014q1 downloaded
arduino:bossac@1.7.0 downloaded
arduino:openocd@0.9.0-arduino6-static downloaded
arduino:CMSIS@4.5.0 downloaded
arduino:CMSIS-Atmel@1.1.0 downloaded
arduino:arduinoOTA@1.2.0 downloaded
Downloading cores...
arduino:samd@1.6.19 downloaded
Installing tools...
Installing platforms...
Results:
arduino:samd@1.6.19 - Installed
arduino:arm-none-eabi-gcc@4.8.3-2014q1 - Installed
arduino:bossac@1.7.0 - Installed
arduino:openocd@0.9.0-arduino6-static - Installed
arduino:CMSIS@4.5.0 - Installed
arduino:CMSIS-Atmel@1.1.0 - Installed
arduino:arduinoOTA@1.2.0 - Installed
```

Now verify we have installed the core properly by running:

```sh
$ arduino-cli core list
ID              Installed       Latest  Name
arduino:samd    1.6.19          1.6.19  Arduino SAMD Boards (32-bits ARM Cortex-M0+)
```

Great! Now we are ready to compile and upload the sketch.

## Adding 3rd party cores

If your board requires 3rd party core packages to work, you can list the URLs to additional package indexes in the
Arduino CLI configuration file.

For example, to add the ESP8266 core, edit the configuration file and change the `board_manager` settings as follows:

```yaml
board_manager:
  additional_urls:
    - https://arduino.esp8266.com/stable/package_esp8266com_index.json
```

If you have your package indexes locally installed, you can list their file path in the Arduino CLI configuration file.

For example, to add the NRF52832 core, edit the configuration file and change the `board_manager` settings as follows:

```yaml
board_manager:
  additional_urls:
    - https://arduino.esp8266.com/stable/package_esp8266com_index.json
    - file:///absolute/path/to/your/package_nrf52832_index.json
```

From now on, commands supporting custom cores will automatically use the additional URL from the configuration file:

```sh
$ arduino-cli core update-index
Updating index: package_index.json downloaded
Updating index: package_esp8266com_index.json downloaded
Updating index: package_nrf52832_index.json
Updating index: package_index.json downloaded

$ arduino-cli core search esp8266
ID              Version Name
esp8266:esp8266 2.5.2   esp8266
```

Alternatively, you can pass a link to the additional package index file with the `--additional-urls` option, that has to
be specified every time and for every command that operates on a 3rd party platform core, for example:

```sh
$ arduino-cli  core update-index --additional-urls https://arduino.esp8266.com/stable/package_esp8266com_index.json
Updating index: package_esp8266com_index.json downloaded

$ arduino-cli core search esp8266 --additional-urls https://arduino.esp8266.com/stable/package_esp8266com_index.json
ID              Version Name
esp8266:esp8266 2.5.2   esp8266
```

The same applies to the additional package index file provided by file paths:

```sh
$ arduino-cli  core update-index --additional-urls file:///absolute/path/to/your/package_esp8266com_index.json
Updating index: package_esp8266com_index.json downloaded

$ arduino-cli core search esp8266 --additional-urls file:///absolute/path/to/your/package_esp8266com_index.json
ID              Version Name
esp8266:esp8266 2.5.2   esp8266
```

## Compile and upload the sketch

To compile the sketch you run the `compile` command, passing the proper FQBN string:

```sh
$ arduino-cli compile --fqbn arduino:samd:mkr1000 MyFirstSketch
Sketch uses 9600 bytes (3%) of program storage space. Maximum is 262144 bytes.
```

To upload the sketch to your board, run the following command, using the serial port your board is connected to:

```sh
$ arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:samd:mkr1000 MyFirstSketch
No new serial port detected.
Atmel SMART device 0x10010005 found
Device       : ATSAMD21G18A
Chip ID      : 10010005
Version      : v2.0 [Arduino:XYZ] Dec 20 2016 15:36:43
Address      : 8192
Pages        : 3968
Page Size    : 64 bytes
Total Size   : 248KB
Planes       : 1
Lock Regions : 16
Locked       : none
Security     : false
Boot Flash   : true
BOD          : true
BOR          : true
Arduino      : FAST_CHIP_ERASE
Arduino      : FAST_MULTI_PAGE_WRITE
Arduino      : CAN_CHECKSUM_MEMORY_BUFFER
Erase flash
done in 0.784 seconds

Write 9856 bytes to flash (154 pages)
[==============================] 100% (154/154 pages)
done in 0.069 seconds

Verify 9856 bytes of flash with checksum.
Verify successful
done in 0.009 seconds
CPU reset.
```

## Add libraries

If you need to add more functionalities to your sketch, chances are some of the libraries available in the Arduino
ecosystem already provide what you need. For example, if you need a debouncing strategy to better handle button inputs,
you can try searching for the `debouncer` keyword:

```sh
$ arduino-cli lib search debouncer
Name: "Debouncer"
    Author: hideakitai
    Maintainer: hideakitai
    Sentence: Debounce library for Arduino
    Paragraph: Debounce library for Arduino
    Website: https://github.com/hideakitai
    Category: Timing
    Architecture: *
    Types: Contributed
    Versions: [0.1.0]
Name: "FTDebouncer"
    Author: Ubi de Feo
    Maintainer: Ubi de Feo, Sebastian Hunkeler
    Sentence: An efficient, low footprint, fast pin debouncing library for Arduino
    Paragraph: This pin state supervisor manages debouncing of buttons and handles transitions between LOW and HIGH state, calling a function and notifying your code of which pin has been activated or deactivated.
    Website: https://github.com/ubidefeo/FTDebouncer
    Category: Uncategorized
    Architecture: *
    Types: Contributed
    Versions: [1.3.0]
Name: "SoftTimer"
    Author: Balazs Kelemen <prampec+arduino@gmail.com>
    Maintainer: Balazs Kelemen <prampec+arduino@gmail.com>
    Sentence: SoftTimer is a lightweight pseudo multitasking solution for Arduino.
    Paragraph: SoftTimer enables higher level Arduino programing, yet easy to use, and lightweight. You are often faced with the problem that you need to do multiple tasks at the same time. In SoftTimer, the programmer creates Tasks that runs periodically. This library comes with a collection of handy tools like blinker, pwm, debouncer.
    Website: https://github.com/prampec/arduino-softtimer
    Category: Timing
    Architecture: *
    Types: Contributed
    Versions: [3.0.0, 3.1.0, 3.1.1, 3.1.2, 3.1.3, 3.1.5, 3.2.0]
```

Our favourite is `FTDebouncer`, let's install it by running:

```sh
$ arduino-cli lib install FTDebouncer
FTDebouncer depends on FTDebouncer@1.3.0
Downloading FTDebouncer@1.3.0...
FTDebouncer@1.3.0 downloaded
Installing FTDebouncer@1.3.0...
Installed FTDebouncer@1.3.0
```

## Using the `daemon` mode and the gRPC interface

Arduino CLI can be launched as a gRPC server via the `daemon` command.

The [client_example] folder contains a sample client code that shows how to interact with the gRPC server. Available
services and messages are detailed in the [gRPC reference] pages.

To provide observability for the gRPC server activities besides logs, the `daemon` mode activates and exposes by default
a [Prometheus](https://prometheus.io/) endpoint (http://localhost:9090/metrics) that can be fetched for metrics data
like:

```text
# TYPE daemon_compile counter
daemon_compile{buildProperties="",exportFile="",fqbn="arduino:samd:mkr1000",installationID="ed6f1f22-1fbe-4b1f-84be-84d035b6369c",jobs="0",libraries="",preprocess="false",quiet="false",showProperties="false",sketchPath="5ff767c6fa5a91230f5cb4e267c889aa61489ab2c4f70f35f921f934c1462cb6",success="true",verbose="true",vidPid="",warnings=""} 1 1580385724726

# TYPE daemon_board_list counter
daemon_board_list{installationID="ed6f1f22-1fbe-4b1f-84be-84d035b6369c",success="true"} 1 1580385724833
```

The metrics settings are exposed via the `metrics` section in the CLI configuration:

```yaml
metrics:
  enabled: true
  addr: :9090
```

[configuration documentation]: configuration.md
[client_example]: https://github.com/arduino/arduino-cli/blob/master/client_example
[grpc reference]: rpc/commands.md
[prometheus]: https://prometheus.io/

## Configuration keys

- `board_manager`
  - `additional_urls` - the URLs to any additional Boards Manager package index files needed for your boards platforms.
- `daemon` - options related to running Arduino CLI as a [gRPC] server.
  - `port` - TCP port used for gRPC client connections.
- `directories` - directories used by Arduino CLI.
  - `data` - directory used to store Boards/Library Manager index files and Boards Manager platform installations.
  - `downloads` - directory used to stage downloaded archives during Boards/Library Manager installations.
  - `user` - the equivalent of the Arduino IDE's ["sketchbook" directory][sketchbook directory]. Library Manager
    installations are made to the `libraries` subdirectory of the user directory.
- `library` - configuration options relating to Arduino libraries.
  - `enable_unsafe_install` - set to `true` to enable the use of the `--git-url` and `--zip-file` flags with
    [`arduino-cli lib install`][arduino cli lib install]. These are considered "unsafe" installation methods because
    they allow installing files that have not passed through the Library Manager submission process.
- `logging` - configuration options for Arduino CLI's logs.
  - `file` - path to the file where logs will be written.
  - `format` - output format for the logs. Allowed values are `text` or `json`.
  - `level` - messages with this level and above will be logged. Valid levels are: `trace`, `debug`, `info`, `warn`,
    `error`, `fatal`, `panic`.
- `metrics` - settings related to the collection of data used for continued improvement of Arduino CLI.
  - `addr` - TCP port used for metrics communication.
  - `enabled` - controls the use of metrics.
- `sketch` - configuration options relating to [Arduino sketches][sketch specification].
  - `always_export_binaries` - set to `true` to make [`arduino-cli compile`][arduino-cli compile] always save binaries
    to the sketch folder. This is the equivalent of using the [`--export-binaries`][arduino-cli compile options] flag.
- `updater` - configuration options related to Arduino CLI updates
  - `enable_notification` - set to `false` to disable notifications of new Arduino CLI releases, defaults to `true`

## Configuration methods

Arduino CLI may be configured in three ways:

1. Command line flags
1. Environment variables
1. Configuration file

If a configuration option is configured by multiple methods, the value set by the method highest on the above list
overwrites the ones below it.

If a configuration option is not set, Arduino CLI uses a default value.

[`arduino-cli config dump`][arduino-cli config dump] displays the current configuration values.

### Command line flags

Arduino CLI's command line flags are documented in the command line help and the [Arduino CLI command reference].

#### Example

Setting an additional Boards Manager URL using the [`--additional-urls`][arduino-cli global flags] command line flag:

```shell
$ arduino-cli core update-index --additional-urls https://downloads.arduino.cc/packages/package_staging_index.json
```

### Environment variables

All configuration options can be set via environment variables. The variable names start with `ARDUINO`, followed by the
configuration key names, with each component separated by `_`. For example, the `ARDUINO_DIRECTORIES_USER` environment
variable sets the `directories.user` configuration option.

On Linux or macOS, you can use the [`export` command][export command] to set environment variables. On Windows cmd, you
can use the [`set` command][set command].

#### Example

Setting an additional Boards Manager URL using the `ARDUINO_BOARD_MANAGER_ADDITIONAL_URLS` environment variable:

```sh
$ export ARDUINO_BOARD_MANAGER_ADDITIONAL_URLS=https://downloads.arduino.cc/packages/package_staging_index.json
```

### Configuration file

[`arduino-cli config init`][arduino-cli config init] creates or updates a configuration file with the current
configuration settings.

This allows saving the options set by command line flags or environment variables. For example:

```sh
arduino-cli config init --additional-urls https://downloads.arduino.cc/packages/package_staging_index.json
```

#### File name

The configuration file must be named `arduino-cli`, with the appropriate file extension for the file's format.

#### Supported formats

`arduino-cli config init` creates a YAML file, however a variety of common formats are supported:

- [JSON]
- [TOML]
- [YAML]
- [Java properties file]
- [HCL]
- envfile
- [INI]

#### Locations

Configuration files in the following locations are recognized by Arduino CLI:

1. Location specified by the [`--config-file`][arduino cli command reference] command line flag
1. Current working directory
1. Any parent directory of the current working directory (more immediate parents having higher precedence)
1. Arduino CLI data directory (as configured by `directories.data`)

If multiple configuration files are present, the one highest on the above list is used. Configuration files are not
combined.

The location of the active configuration file can be determined by running the command:

```sh
arduino-cli config dump --verbose
```

#### Example

Setting an additional Boards Manager URL using a YAML format configuration file:

```yaml
board_manager:
  additional_urls:
    - https://downloads.arduino.cc/packages/package_staging_index.json
```

Doing the same using a TOML format file:

```toml
[board_manager]
additional_urls = [ "https://downloads.arduino.cc/packages/package_staging_index.json" ]
```

[grpc]: https://grpc.io
[sketchbook directory]: sketch-specification.md#sketchbook
[arduino cli lib install]: commands/arduino-cli_lib_install.md
[sketch specification]: sketch-specification.md
[arduino-cli compile]: commands/arduino-cli_compile.md
[arduino-cli compile options]: commands/arduino-cli_compile.md#options
[arduino-cli config dump]: commands/arduino-cli_config_dump.md
[arduino cli command reference]: commands/arduino-cli.md
[arduino-cli global flags]: commands/arduino-cli_config.md#options-inherited-from-parent-commands
[export command]: https://ss64.com/bash/export.html
[set command]: https://docs.microsoft.com/en-us/windows-server/administration/windows-commands/set_1
[arduino-cli config init]: commands/arduino-cli_config_init.md
[json]: https://www.json.org
[toml]: https://github.com/toml-lang/toml
[yaml]: https://en.wikipedia.org/wiki/YAML
[java properties file]: https://en.wikipedia.org/wiki/.properties
[hcl]: https://github.com/hashicorp/hcl
[ini]: https://en.wikipedia.org/wiki/INI_file

This is the specification for Arduino sketches.

The programs that run on Arduino boards are called "sketches". This term was inherited from
[Processing](https://processing.org/), upon which the Arduino IDE and the core API were based.

## Sketch folders and files

The sketch root folder name and code file names must start with a basic letter (`A`-`Z` or `a`-`z`) or number (`0`-`9`),
followed by basic letters, numbers, underscores (`_`), dots (`.`) and dashes (`-`). The maximum length is 63 characters.

Support for names starting with a number was added in Arduino IDE 1.8.4.

### Sketch root folder

Because many Arduino sketches only contain a single .ino file, it's easy to think of that file as the sketch. However,
it is the folder that is the sketch. The reason is that sketches may consist of multiple code files and the folder is
what groups those files into a single program.

### Primary sketch file

Every sketch must contain a `.ino` file with a file name matching the sketch root folder name.

`.pde` is also supported but **deprecated** and will be removed in the future, using the `.ino` extension is strongly
recommended.

### Additional code files

Sketches may consist of multiple code files.

The following extensions are supported:

- .ino - [Arduino language](https://www.arduino.cc/reference/en/) files.
- .pde - Alternate extension for Arduino language files. This file extension is also used by Processing sketches. .ino
  is recommended to avoid confusion. **`.pde` extension is deprecated and will be removed in the future.**
- .cpp - C++ files.
- .c - C Files.
- .S - Assembly language files.
- .h - Header files.
- .tpp, .ipp - Header files (available from Arduino CLI 0.19.0).

For information about how each of these files and other parts of the sketch are used during compilation, see the
[Sketch build process documentation](sketch-build-process.md).

### `src` subfolder

The contents of the `src` subfolder are compiled recursively. Unlike the code files in the sketch root folder, these
files are not shown as tabs in the IDEs.

This is useful for files you don't want to expose to the sketch user via the IDE's interface. It can be used to bundle
libraries with the sketch in order to make it a self-contained project.

Arduino language files under the `src` folder are not supported.

- In Arduino IDE 1.6.5-r5 and older, no recursive compilation was done.
- In Arduino IDE 1.6.6 - 1.6.9, recursive compilation was done of all subfolders of the sketch folder.
- In Arduino IDE 1.6.10 and newer, recursive compilation is limited to the `src` subfolder of the sketch folder.

### `data` subfolder

The `data` folder is used to add additional files to the sketch, which will not be compiled.

Files added to the sketch via the Arduino IDE's **Sketch > Add File...** are placed in the `data` folder.

The Arduino IDE's **File > Save As...** only copies the code files in the sketch root folder and the full contents of
the `data` folder, so any non-code files outside the `data` folder are stripped.

### Metadata

Arduino CLI and Arduino Web Editor use a file named sketch.json, located in the sketch root folder, to store sketch
metadata.

The `cpu` key contains the board configuration information. This can be set via
[`arduino-cli board attach`](commands/arduino-cli_board_attach.md) or by selecting a board in the Arduino Web Editor
while the sketch is open. With this configuration set, it is not necessary to specify the `--fqbn` or `--port` flags to
the [`arduino-cli compile`](commands/arduino-cli_compile.md) or [`arduino-cli upload`](commands/arduino-cli_upload.md)
commands when compiling or uploading the sketch.

The `included_libs` key defines the library versions the Arduino Web Editor uses when the sketch is compiled. This is
Arduino Web Editor specific because all versions of all the Library Manager libraries are pre-installed in Arduino Web
Editor, while only one version of each library may be installed when using the other Arduino development software.

### Secrets

Arduino Web Editor has a
["Secret tab" feature](https://create.arduino.cc/projecthub/Arduino_Genuino/store-your-sensitive-data-safely-when-sharing-a-sketch-e7d0f0)
that makes it easy to share sketches without accidentally exposing sensitive data (e.g., passwords or tokens). The
Arduino Web Editor automatically generates macros for any identifier in the sketch which starts with `SECRET_` and
contains all uppercase characters.

When you download a sketch from Arduino Web Editor that contains a Secret tab, the empty `#define` directives for the
secrets are in a file named arduino_secrets.h, with an `#include` directive to that file at the top of the primary
sketch file. This is hidden when viewing the sketch in Arduino Web Editor.

### Documentation

Image and text files in common formats which are present in the sketch root folder are displayed in tabs in the Arduino
Web Editor.

### Sketch file structure example

```
Foo
|_ arduino_secrets.h
|_ Abc.ino
|_ Def.cpp
|_ Def.h
|_ Foo.ino
|_ Ghi.c
|_ Ghi.h
|_ Jkl.h
|_ Jkl.S
|_ sketch.json
|_ data
|  |_ Schematic.pdf
|_ src
   |_ SomeLib
      |_ library.properties
      |_ src
         |_ SomeLib.h
         |_ SomeLib.cpp
```

## Sketchbook

The Arduino IDE provides a "sketchbook" folder (analogous to Arduino CLI's "user directory"). In addition to being the
place where user libraries and manually installed platforms are installed, the sketchbook is a convenient place to store
sketches. Sketches in the sketchbook folder appear under the Arduino IDE's **File > Sketchbook** menu. However, there is
no requirement to store sketches in the sketchbook folder.

## Library/Boards Manager links

A URI in a comment in the form `http://librarymanager#SEARCH_TERM` will open a search for SEARCH_TERM in
[Library Manager](https://www.arduino.cc/en/guide/libraries#toc3) when clicked in the Arduino IDE.

A URI in a comment in the form `http://boardsmanager#SEARCH_TERM` will open a search for SEARCH_TERM in
[Boards Manager](https://www.arduino.cc/en/Guide/Cores) when clicked in the Arduino IDE.

This can be used to offer the user an easy way to install dependencies of the sketch.

This feature is only available when using the Arduino IDE, so be sure to provide supplementary documentation to help the
users of other development software install the sketch dependencies.

This feature was added in Arduino IDE 1.6.9.

### Example

```c++
// install the Arduino SAMD Boards platform to add support for your MKR WiFi 1010 board
// if using the Arduino IDE, click here: http://boardsmanager#SAMD

// install the WiFiNINA library via Library Manager
// if using the Arduino IDE, click here: http://librarymanager#WiFiNINA
#include <WiFiNINA.h>
```

## See also

- [Sketch build process documentation](sketch-build-process.md)
- [Style guide for example sketches](https://www.arduino.cc/en/Reference/StyleGuide)

## Boards

gustavocampos@GUSTAVOs-MBP .ssh % arduino-cli core list
ID              Installed Latest Name
arduino:avr     1.8.3     1.8.4  Arduino AVR Boards
arduino:megaavr 1.8.7     1.8.7  Arduino megaAVR Boards
arduino:sam     1.6.12    1.6.12 Arduino SAM Boards (32-bits ARM Cortex-M3)
arduino:samd    1.8.11    1.8.12 Arduino SAMD Boards (32-bits ARM Cortex-M0+)
esp32:esp32     1.0.6     1.0.6  ESP32 Arduino
esp8266:esp8266 3.0.2     3.0.2  ESP8266 Boards (3.0.2)
Maixduino:k210  0.3.11    0.3.11 Maixduino

## repos

https://raw.githubusercontent.com/sparkfun/Arduino_Boards/master/IDE_Board_Manager/package_sparkfun_index.json
https://dl.espressif.com/dl/package_esp32_index.json
http://arduino.esp8266.com/stable/package_esp8266com_index.json
https://raw.githubusercontent.com/stm32duino/BoardManagerFiles/master/package_stmicroelectronics_index.json
https://raw.githubusercontent.com/damellis/attiny/ide-1.6.x-boards-manager/package_damellis_attiny_index.json
http://dl.sipeed.com/MAIX/Maixduino/package_Maixduino_k210_index.json


## Full TODAY's boards

```
Board Name                          FQBN
3D printer boards                   STMicroelectronics:stm32:3dprinter
4D Systems gen4 IoD Range           esp8266:esp8266:gen4iod
AI Thinker ESP32-CAM                esp32:esp32:esp32cam
ALKS ESP32                          esp32:esp32:alksesp32
ATtiny24/44/84                      attiny:avr:ATtinyX4
ATtiny25/45/85                      attiny:avr:ATtinyX5
Adafruit Circuit Playground         arduino:avr:circuitplay32u4cat
Adafruit Circuit Playground Express arduino:samd:adafruit_circuitplayground_m0
Adafruit ESP32 Feather              esp32:esp32:featheresp32
Adafruit Feather HUZZAH ESP8266     esp8266:esp8266:huzzah
Amperka WiFi Slot                   esp8266:esp8266:wifi_slot
Arduino                             esp8266:esp8266:arduino-esp8266
Arduino BT                          arduino:avr:bt
Arduino Due (Native USB Port)       arduino:sam:arduino_due_x
Arduino Due (Programming Port)      arduino:sam:arduino_due_x_dbg
Arduino Duemilanove or Diecimila    arduino:avr:diecimila
Arduino Esplora                     arduino:avr:esplora
Arduino Ethernet                    arduino:avr:ethernet
Arduino Fio                         arduino:avr:fio
Arduino Gemma                       arduino:avr:gemma
Arduino Industrial 101              arduino:avr:chiwawa
Arduino Leonardo                    arduino:avr:leonardo
Arduino Leonardo ETH                arduino:avr:leonardoeth
Arduino M0                          arduino:samd:mzero_bl
Arduino M0 Pro (Native USB Port)    arduino:samd:mzero_pro_bl
Arduino M0 Pro (Programming Port)   arduino:samd:mzero_pro_bl_dbg
Arduino MKR FOX 1200                arduino:samd:mkrfox1200
Arduino MKR GSM 1400                arduino:samd:mkrgsm1400
Arduino MKR NB 1500                 arduino:samd:mkrnb1500
Arduino MKR Vidor 4000              arduino:samd:mkrvidor4000
Arduino MKR WAN 1300                arduino:samd:mkrwan1300
Arduino MKR WAN 1310                arduino:samd:mkrwan1310
Arduino MKR WiFi 1010               arduino:samd:mkrwifi1010
Arduino MKR1000                     arduino:samd:mkr1000
Arduino MKRZERO                     arduino:samd:mkrzero
Arduino Mega ADK                    arduino:avr:megaADK
Arduino Mega or Mega 2560           arduino:avr:mega
Arduino Micro                       arduino:avr:micro
Arduino Mini                        arduino:avr:mini
Arduino NANO 33 IoT                 arduino:samd:nano_33_iot
Arduino NG or older                 arduino:avr:atmegang
Arduino Nano                        arduino:avr:nano
Arduino Nano 33 BLE                 arduino:mbed_nano:nano33ble
Arduino Nano Every                  arduino:megaavr:nona4809
Arduino Nano RP2040 Connect         arduino:mbed_nano:nanorp2040connect
Arduino Pro or Pro Mini             arduino:avr:pro
Arduino Robot Control               arduino:avr:robotControl
Arduino Robot Motor                 arduino:avr:robotMotor
Arduino Tian                        arduino:samd:tian
Arduino Uno                         arduino:avr:uno
Arduino Uno Mini                    arduino:avr:unomini
Arduino Uno WiFi                    arduino:avr:unowifi
Arduino Uno WiFi Rev2               arduino:megaavr:uno2018
Arduino Yún                         arduino:avr:yun
Arduino Yún Mini                    arduino:avr:yunmini
Arduino Zero (Native USB Port)      arduino:samd:arduino_zero_native
Arduino Zero (Programming Port)     arduino:samd:arduino_zero_edbg
BPI-BIT                             esp32:esp32:bpi-bit
Blues Wireless boards               STMicroelectronics:stm32:BluesW
D-duino-32                          esp32:esp32:d-duino-32
DOIT ESP-Mx DevKit (ESP8285)        esp8266:esp8266:espmxdevkit
DOIT ESP32 DEVKIT V1                esp32:esp32:esp32doit-devkit-v1
DOIT ESPduino32                     esp32:esp32:esp32doit-espduino
Digistump Oak                       esp8266:esp8266:oak
Discovery                           STMicroelectronics:stm32:Disco
Dongsen Tech Pocket 32              esp32:esp32:pocket_32
ESP32 Dev Module                    esp32:esp32:esp32
ESP32 FM DevKit                     esp32:esp32:fm-devkit
ESP32 Pico Kit                      esp32:esp32:pico32
ESP32 Wrover Module                 esp32:esp32:esp32wrover
ESP32vn IoT Uno                     esp32:esp32:esp32vn-iot-uno
ESPDuino (ESP-13 Module)            esp8266:esp8266:espduino
ESPea32                             esp32:esp32:espea32
ESPectro Core                       esp8266:esp8266:espectro
ESPectro32                          esp32:esp32:espectro32
ESPino (ESP-12 Module)              esp8266:esp8266:espino
ESPresso Lite 1.0                   esp8266:esp8266:espresso_lite_v1
ESPresso Lite 2.0                   esp8266:esp8266:espresso_lite_v2
ET-Board                            esp32:esp32:ET-Board
Elecgator boards                    STMicroelectronics:stm32:Elecgator
Electronic SweetPeas - ESP320       esp32:esp32:esp320
Electronic speed controllers        STMicroelectronics:stm32:ESC_board
Eval                                STMicroelectronics:stm32:Eval
FireBeetle-ESP32                    esp32:esp32:firebeetle32
Frog Board ESP32                    esp32:esp32:frogboard
Garatronic-McHobby                  STMicroelectronics:stm32:Garatronic
Generic ESP8266 Module              esp8266:esp8266:generic
Generic ESP8285 Module              esp8266:esp8266:esp8285
Generic Flight Controllers          STMicroelectronics:stm32:GenFlight
Generic STM32F0 series              STMicroelectronics:stm32:GenF0
Generic STM32F1 series              STMicroelectronics:stm32:GenF1
Generic STM32F2 series              STMicroelectronics:stm32:GenF2
Generic STM32F3 series              STMicroelectronics:stm32:GenF3
Generic STM32F4 series              STMicroelectronics:stm32:GenF4
Generic STM32F7 series              STMicroelectronics:stm32:GenF7
Generic STM32G0 series              STMicroelectronics:stm32:GenG0
Generic STM32G4 series              STMicroelectronics:stm32:GenG4
Generic STM32H7 Series              STMicroelectronics:stm32:GenH7
Generic STM32L0 series              STMicroelectronics:stm32:GenL0
Generic STM32L1 series              STMicroelectronics:stm32:GenL1
Generic STM32L4 series              STMicroelectronics:stm32:GenL4
Generic STM32L5 series              STMicroelectronics:stm32:GenL5
Generic STM32U5 series              STMicroelectronics:stm32:GenU5
Generic STM32WB series              STMicroelectronics:stm32:GenWB
Generic STM32WL series              STMicroelectronics:stm32:GenWL
HONEYLemon                          esp32:esp32:honeylemon
Heltec WiFi Kit 32                  esp32:esp32:heltec_wifi_kit_32
Heltec WiFi LoRa 32                 esp32:esp32:heltec_wifi_lora_32
Heltec WiFi LoRa 32(V2)             esp32:esp32:heltec_wifi_lora_32_V2
Heltec Wireless Stick               esp32:esp32:heltec_wireless_stick
Heltec Wireless Stick Lite          esp32:esp32:heltec_wireless_stick_lite
Hornbill ESP32 Dev                  esp32:esp32:hornbill32dev
Hornbill ESP32 Minima               esp32:esp32:hornbill32minima
IMBRIOS LOGSENS_V1P1                esp32:esp32:imbrios-logsens-v1p1
INEX OpenKB                         esp32:esp32:OpenKB
ITEAD Sonoff                        esp8266:esp8266:sonoff
IntoRobot Fig                       esp32:esp32:intorobot-fig
Invent One                          esp8266:esp8266:inventone
KITS ESP32 EDU                      esp32:esp32:kits-edu
LOLIN D32                           esp32:esp32:d32
LOLIN D32 PRO                       esp32:esp32:d32_pro
LOLIN(WEMOS) D1 R2 & mini           esp8266:esp8266:d1_mini
LOLIN(WEMOS) D1 mini (clone)        esp8266:esp8266:d1_mini_clone
LOLIN(WEMOS) D1 mini Lite           esp8266:esp8266:d1_mini_lite
LOLIN(WEMOS) D1 mini Pro            esp8266:esp8266:d1_mini_pro
LOLIN(WeMos) D1 R1                  esp8266:esp8266:d1
Labplus mPython                     esp32:esp32:mPython
LamLoei AIoT DaaN Board             Maixduino:k210:aiotdaan
Lifely Agrumino Lemon v4            esp8266:esp8266:agruminolemon
LilyPad Arduino                     arduino:avr:lilypad
LilyPad Arduino USB                 arduino:avr:LilyPadUSB
Linino One                          arduino:avr:one
LoPy                                esp32:esp32:lopy
LoPy4                               esp32:esp32:lopy4
LoRa boards                         STMicroelectronics:stm32:LoRa
M5Stack-ATOM                        esp32:esp32:m5stack-atom
M5Stack-Core-ESP32                  esp32:esp32:m5stack-core-esp32
M5Stack-Core2                       esp32:esp32:m5stack-core2
M5Stack-CoreInk                     esp32:esp32:m5stack-coreink
M5Stack-FIRE                        esp32:esp32:m5stack-fire
M5Stack-Timer-CAM                   esp32:esp32:m5stack-timer-cam
M5Stick-C                           esp32:esp32:m5stick-c
MGBOT IOTIK 32A                     esp32:esp32:mgbot-iotik32a
MGBOT IOTIK 32B                     esp32:esp32:mgbot-iotik32b
MH ET LIVE ESP32DevKIT              esp32:esp32:mhetesp32devkit
MH ET LIVE ESP32MiniKit             esp32:esp32:mhetesp32minikit
MagicBit                            esp32:esp32:magicbit
Metro ESP-32                        esp32:esp32:metro_esp-32
Microduino-CoreESP32                esp32:esp32:CoreESP32
Midatronics boards                  STMicroelectronics:stm32:Midatronics
Nano32                              esp32:esp32:nano32
Node32s                             esp32:esp32:node32s
NodeMCU 0.9 (ESP-12 Module)         esp8266:esp8266:nodemcu
NodeMCU 1.0 (ESP-12E Module)        esp8266:esp8266:nodemcuv2
NodeMCU-32S                         esp32:esp32:nodemcu-32s
Noduino Quantum                     esp32:esp32:quantum
Nucleo-144                          STMicroelectronics:stm32:Nucleo_144
Nucleo-32                           STMicroelectronics:stm32:Nucleo_32
Nucleo-64                           STMicroelectronics:stm32:Nucleo_64
ODROID ESP32                        esp32:esp32:odroid_esp32
OLIMEX ESP32-DevKit-LiPo            esp32:esp32:esp32-DevKitLipo
OLIMEX ESP32-EVB                    esp32:esp32:esp32-evb
OLIMEX ESP32-GATEWAY                esp32:esp32:esp32-gateway
OLIMEX ESP32-PoE                    esp32:esp32:esp32-poe
OLIMEX ESP32-PoE-ISO                esp32:esp32:esp32-poe-iso
OROCA EduBot                        esp32:esp32:oroca_edubot
Olimex MOD-WIFI-ESP8266(-DEV)       esp8266:esp8266:modwifi
Onehorse ESP32 Dev Module           esp32:esp32:onehorse32dev
Phoenix 1.0                         esp8266:esp8266:phoenix_v1
Phoenix 2.0                         esp8266:esp8266:phoenix_v2
Piranha ESP-32                      esp32:esp32:piranha_esp-32
ProtoCentral HealthyPi 4            esp32:esp32:healthypi4
Pycom GPy                           esp32:esp32:gpy
S.ODI Ultra v1                      esp32:esp32:S_ODI_Ultra
STM32MP1 series coprocessor         STMicroelectronics:stm32:STM32MP1
Schirmilabs Eduino WiFi             esp8266:esp8266:eduinowifi
Seeed Wio Link                      esp8266:esp8266:wiolink
Senses's WEIZEN                     esp32:esp32:sensesiot_weizen
Silicognition wESP32                esp32:esp32:wesp32
Sipeed Maix Bit Board               Maixduino:k210:bit
Sipeed Maix Bit-Mic Board           Maixduino:k210:bitm
Sipeed Maix Go Board                Maixduino:k210:go
Sipeed Maix One Dock Board          Maixduino:k210:m1
Sipeed Maixduino Board              Maixduino:k210:mduino
SparkFun Blynk Board                esp8266:esp8266:blynk
SparkFun ESP32 Thing                esp32:esp32:esp32thing
SparkFun ESP32 Thing Plus           esp32:esp32:esp32thing_plus
SparkFun ESP8266 Thing              esp8266:esp8266:thing
SparkFun ESP8266 Thing Dev          esp8266:esp8266:thingdev
SparkFun LoRa Gateway 1-Channel     esp32:esp32:sparkfun_lora_gateway_1-channel
SweetPea ESP-210                    esp8266:esp8266:esp210
T-Beam                              esp32:esp32:t-beam
TTGO LoRa32-OLED V1                 esp32:esp32:ttgo-lora32-v1
TTGO LoRa32-OLED v2.1.6             esp32:esp32:ttgo-lora32-v21new
TTGO T-Watch                        esp32:esp32:twatch
TTGO T1                             esp32:esp32:ttgo-t1
TTGO T7 V1.3 Mini32                 esp32:esp32:ttgo-t7-v13-mini32
TTGO T7 V1.4 Mini32                 esp32:esp32:ttgo-t7-v14-mini32
ThaiEasyElec's ESPino               esp8266:esp8266:espinotee
ThaiEasyElec's ESPino32             esp32:esp32:espino32
TinyPICO                            esp32:esp32:tinypico
Turta IoT Node                      esp32:esp32:turta_iot_node
VintLabs ESP32 Devkit               esp32:esp32:vintlabs-devkit-v1
WEMOS D1 MINI ESP32                 esp32:esp32:d1_mini32
WEMOS LOLIN32                       esp32:esp32:lolin32
WEMOS LOLIN32 Lite                  esp32:esp32:lolin32-lite
WeMos WiFi&Bluetooth Battery        esp32:esp32:WeMosBat
WiFi Kit 8                          esp8266:esp8266:wifi_kit_8
WiFiduino                           esp8266:esp8266:wifiduino
WiFiduino32                         esp32:esp32:wifiduino32
WiPy 3.0                            esp32:esp32:wipy3
Widora AIR                          esp32:esp32:widora-air
WifInfo                             esp8266:esp8266:wifinfo
XinaBox CW01                        esp8266:esp8266:cw01
XinaBox CW02                        esp32:esp32:cw02
u-blox NINA-W10 series (ESP32)      esp32:esp32:nina_w10
```

## Hints for bluepill STM32F103

Exemple compilation/upload:

`arduino-cli compile -v -b STMicroelectronics:stm32:GenF1:pnum=BLUEPILL_F103C8,usb=CDCgen -upload -p "." .`

If you see the error: `Error during Upload: Property 'upload.tool.serial' is undefined`
please, Add the following line anywhere in the STM32 boards.txt file, otherwise:
`GenF1.menu.upload_method.swdMethod.upload.tool.default=stm32CubeProg`