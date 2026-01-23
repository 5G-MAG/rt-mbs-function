<h1 align="center">5G MBS User Services: MBS Function</h1>
<p align="center">
  <img src="https://img.shields.io/github/v/tag/5G-MAG/rt-mbs-function?label=version" alt="Version">
  <img src="https://img.shields.io/badge/Status-Under_Development-yellow" alt="Under Development">
  <img src="https://img.shields.io/badge/License-5G--MAG%20Public%20License%20(v1.0)-blue" alt="License">
</p>

## Introduction

This repository provides a 5G MBS Function which forms part of the MBS User Services. This NF provides the interface designated as Nmb10 in the [3GPP TS 29.580](https://www.3gpp.org/DynaReport/29580.htm) specification.

Additional information can be found at: https://5g-mag.github.io/Getting-Started/pages/

## Install dependencies

Please use a linux distribution with GCC 14 or later (e.g. Ubuntu 24.04 or later) as this release requires C++ features that were initially implemented in GCC version 14.

```bash
sudo add-apt-repository universe
sudo apt update
sudo apt install git ninja-build build-essential flex bison libsctp-dev libgnutls28-dev libgcrypt-dev libssl-dev libidn11-dev libmongoc-dev libbson-dev libyaml-dev libnghttp2-dev libmicrohttpd-dev libcurl4-gnutls-dev libtins-dev libtalloc-dev libpcre2-dev curl wget default-jdk cmake jq util-linux-extra
sudo python3 -m pip install --upgrade meson
```

## Downloading

Release tar files can be downloaded from <https://github.com/5G-MAG/rt-mbs-function/releases>.

The source can be obtained by cloning the GitHub repository.

For example to download the latest release you can use:

```bash
cd ~
git clone --recurse-submodules https://github.com/5G-MAG/rt-mbs-function.git
```

## Building

The build process requires a working Internet connection as the API files are retrieved at build time.

To build the 5G MBS Function from the source:

```bash
cd ~/rt-mbs-function
meson build
ninja -C build
```

**Note:** Errors during the `meson build` command are often caused by missing dependencies or a network issue while trying to retrieve the API files and `openapi-generator` JAR file. See the `~/rt-mbs-function/build/meson-logs/meson-log.txt` log file for the errors in greater detail. Search for `generator-mbsf` to find the start of the API fetch sequence.

## Regression tests (optional)

There are some regression tests that can be run using:

```bash
cd ~/rt-mbs-function
meson test -C build regression
```

This will build the MBSF (if not already built) and then will start an Open5GS NRF and the MBSF, run the regression tests and
shutdown the Open5GS NRF and MBSF. The results of the testing are displayed.

## Installing

To install the built MBS Function as a system process:

```bash
cd ~/rt-mbs-function/build
sudo meson install --no-rebuild
```

## Running

The MBS Function requires a running 5G Core NRF Network Function to register with. If you do not have a running 5G Core, the [Open5GS](https://open5gs.org/) Network Functions are installed as part of the installation procedure and the Open5GS NRF can be started using:

```bash
sudo /usr/local/bin/open5gs-nrfd &
```

Make sure the IP address and port details of the NRF you are running are configured in the `nrf` section of `/usr/local/etc/open5gs/mbsf.yaml` and then run the MBS Function. For example:

```bash
sudo /usr/local/bin/open5gs-mbsfd &
```

## Development

This project follows
the [Gitflow workflow](https://www.atlassian.com/git/tutorials/comparing-workflows/gitflow-workflow). The
`development` branch of this project serves as an integration branch for new features. Consequently, please make sure to
switch to the `development` branch before starting the implementation of a new feature.

