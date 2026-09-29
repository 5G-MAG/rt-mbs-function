<p align="center">
  <img src=".github/banner.svg" width="100%" alt="Reference Tools · 5G Multicast Broadcast Services (MBS): MBS Function (MBSF)">
</p>

<p align="center">
  An MBS Function (MBSF) for the 5G MBS User Services, providing the Nmb10 interface specified in
  3GPP TS 29.580.
</p>

<p align="center">
  <img alt="Status: Under Development"
    src="https://img.shields.io/badge/Status-Under%20Development-e67e22">
  <a href="https://github.com/5G-MAG/rt-mbs-function/releases"><img alt="Version"
    src="https://img.shields.io/github/v/release/5G-MAG/rt-mbs-function?label=Version"></a>
  <a href="LICENSE"><img alt="License: 5G-MAG Public License v1.0"
    src="https://img.shields.io/badge/License-5G--MAG%20PL%20v1.0-blue"></a>
</p>

<p align="center">
  <a href="https://www.5g-mag.com/reference-tools/5g-mbs/">Project page</a> &nbsp;&middot;&nbsp;
  <a href="https://github.com/5G-MAG/rt-mbs-function/issues">Issues</a> &nbsp;&middot;&nbsp;
  <a href="https://www.5g-mag.com/contributing">Contributing</a>
</p>

---

## At a glance

|  |  |
|---|---|
| **Implements** | [3GPP TS 29.580](https://www.3gpp.org/DynaReport/29580.htm), the Nmbsf APIs on the Nmb10 interface, Release 18 |
| **Part of** | [5G Multicast Broadcast Services (MBS)](https://www.5g-mag.com/reference-tools/5g-mbs/), alongside [open5gs](https://github.com/5G-MAG/open5gs), [rt-5gc-service-consumers](https://github.com/5G-MAG/rt-5gc-service-consumers), [rt-libflute](https://github.com/5G-MAG/rt-libflute), [rt-mbs-application](https://github.com/5G-MAG/rt-mbs-application), [rt-mbs-application-provider](https://github.com/5G-MAG/rt-mbs-application-provider), [rt-mbs-client](https://github.com/5G-MAG/rt-mbs-client), [rt-mbs-examples](https://github.com/5G-MAG/rt-mbs-examples), [rt-mbs-transport-function](https://github.com/5G-MAG/rt-mbs-transport-function), [rt-media-origin](https://github.com/5G-MAG/rt-media-origin), [rt-srsRAN_Project](https://github.com/5G-MAG/rt-srsRAN_Project), [srsRAN_4G](https://github.com/5G-MAG/srsRAN_4G), [srsRAN_4G_mbs](https://github.com/5G-MAG/srsRAN_4G_mbs) and [srsRAN_Project_mbs](https://github.com/5G-MAG/srsRAN_Project_mbs) |

## Introduction

The MBS Function is the network function of the MBS User Services that an MBS Application Provider
uses, over Nmb10, to provision MBS User Services and their data ingest sessions. It uses the Nmb2
interface to the MBS Transport Function
([rt-mbs-transport-function](https://github.com/5G-MAG/rt-mbs-transport-function)), and is built as
an [Open5GS](https://open5gs.org/) network function that registers with a 5G Core NRF.

## Specification

Built against 3GPP TS 29.580, Release 18. The API bindings are generated at build time from the
3GPP 5G APIs, by default at tag `TSG111-Rel18` (build options `fiveg_api_release` and
`fiveg_api_approval` in `meson_options.txt`). Besides the TS 29.580 Nmbsf APIs, the build uses the
TS 29.581 Nmbstf distribution session API and the TS 26.517 service announcement and object manifest
models.

Clause-by-clause coverage, and what is still absent, is recorded on the project page rather than
here: <https://www.5g-mag.com/reference-tools/5g-mbs/>

## Install dependencies

Use a Linux distribution with GCC 14 or later (for example Ubuntu 24.04 or later): this release
needs C++ features first implemented in GCC 14.

```bash
sudo add-apt-repository universe
sudo apt update
sudo apt install git ninja-build build-essential flex bison libsctp-dev libgnutls28-dev libgcrypt-dev libssl-dev libidn11-dev libmongoc-dev libbson-dev libyaml-dev libnghttp2-dev libmicrohttpd-dev libcurl4-gnutls-dev libtins-dev libtalloc-dev libpcre2-dev uuid-dev curl wget default-jdk cmake jq util-linux-extra meson
```

## Downloading

Release tar files are available from <https://github.com/5G-MAG/rt-mbs-function/releases>.

Alternatively, clone the repository with its submodules. The default branch holds the latest
release:

```bash
cd ~
git clone --recurse-submodules https://github.com/5G-MAG/rt-mbs-function.git
```

## Building

The build needs a working Internet connection, because the API files are retrieved at build time.

To build the MBS Function from source:

```bash
cd ~/rt-mbs-function
meson build
ninja -C build
```

Errors during `meson build` are usually caused by missing dependencies, or by a network problem while
retrieving the API files and the `openapi-generator` JAR file. The details are in
`~/rt-mbs-function/build/meson-logs/meson-log.txt`; search it for `generator-mbsf` to find the start
of the API fetch sequence.

### Wrong Meson version

If the Meson version installed with `apt` is older than the project requires, the build fails with an
error like this:

```text
meson.build:12:20: ERROR: Meson version is 1.3.2 but project requires >= 1.4.0
```

In that case, remove the APT package and install the latest Meson with `pip`:

```bash
sudo apt-get remove meson
sudo python3 -m pip install --break-system-packages --upgrade meson
```

## Installing

To install the MBS Function as a system process:

```bash
cd ~/rt-mbs-function/build
sudo meson install --no-rebuild
```

## Running

The MBS Function needs a running 5G Core NRF to register with. If you do not have a 5G Core, the
installation also installs the [Open5GS](https://open5gs.org/) network functions, and you can start
the Open5GS NRF with:

```bash
sudo /usr/local/bin/open5gs-nrfd &
```

Set the IP address and port of your NRF in the `nrf` section of `/usr/local/etc/open5gs/mbsf.yaml`,
then start the MBS Function, for example:

```bash
sudo /usr/local/bin/open5gs-mbsfd &
```

## Development

This project follows the
[Gitflow workflow](https://www.atlassian.com/git/tutorials/comparing-workflows/gitflow-workflow). The
`development` branch is the integration branch for new features: switch to it before starting work on
a new feature.

## Contributing

Contributions are welcome. How to raise an issue, fork the repository and open a pull request, and
the Contributor License Agreement required before code can be merged, are described at
<https://www.5g-mag.com/contributing>.

## License

Distributed under the 5G-MAG Public License v1.0. See [LICENSE](LICENSE).
