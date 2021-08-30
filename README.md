# Hello World Example

Starts a FreeRTOS task to interface with a mitubishi heat pump through its serial interface.

User configuration occurs through an http web interface or via an MQTT broker.

## Build Instructions
```
$ idf.py build
$ idf.py flash
$ idf.py monitor
```

or alternatively

```
$ mkdir build && cd build
$ cmake ..
$ make
$ make flash
```

Before building, some configuration may be performed by running

```
$ idf.py menuconfig
```

## Folder contents

This project is built using CMake. The project build configuration is contained in `CMakeLists.txt` files.

Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   └── mitsusplit_main.c
└── README.md                  This is the file you are currently reading
```

For more information on structure and contents of ESP-IDF projects, please refer to Section [Build System](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html) of the ESP-IDF Programming Guide.

## Troubleshooting

* Program upload failure

    * Hardware connection is not correct: run `idf.py -p PORT monitor`, and reboot your board to see if there are any output logs.
    * The baud rate for downloading is too high: lower your baud rate in the `menuconfig` menu, and try again.

