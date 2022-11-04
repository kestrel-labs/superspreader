# superspreader
![ci badge](https://github.com/kestrel-labs/superspreader/actions/workflows/ci.yaml/badge.svg)
[![Doxygen](https://github.com/kestrel-labs/superspreader/actions/workflows/doxygen.yaml/badge.svg?branch=main)](https://github.com/kestrel-labs/superspreader/actions/workflows/doxygen.yaml?query=branch%3Amain)
[![license](https://img.shields.io/github/license/kestrel-labs/superspreader)](https://github.com/kestrel-labs/superspreader/blob/main/LICENSE)


# Quickstart
## Build
Build a new development image
```shell
export UID=$(id -u) export GID=$(id -g); docker compose -f compose.dev.yml build
```
Start an interactive development container
```shell
docker compose -f compose.dev.yml run development
```
Build and upload the health monitor:
```shell
username@superspreader-dev:~/ws arduino-cli compile --fqbn esp32:esp32:esp32 src/superspreader/arduino/health_monitor
username@superspreader-dev:~/ws arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 src/superspreader/arduino/health_monitor
username@superspreader-dev:~/ws arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

On the host, be sure to run these commands so the container
has permissions to upload to the hardware.
```shell
sudo chmod 666 /dev/ttyUSB0
sudo chown $USER /dev/ttyUSB0
sudo adduser $USER dialout
```
