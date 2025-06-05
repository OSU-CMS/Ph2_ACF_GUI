# Installing rarpd

The rarpd (Reverse Address Resolution Protocol daemon) responds to RARP requests.
RARP is used by some machines such as the FC7 at boot time to discover their IP address.
They provide their Ethernet address (see
[list of FC7 MAC addresses](https://cms-tracker-daq.web.cern.ch/cms-tracker-daq/components/300_fc7_macs/))
and rarpd responds with their IP address if it finds it in the ethers database (here `/etc/ethers`).

## Obtaining and installing rarpd package

There is currently no `rarpd` package available specifically for AlmaLinux 9.
However, the Fedora version can be used and is available from
<https://pkgs.org/download/rarpd>.
Make sure to choose Fedora 39 and `x86_64`.
Direct download link:
<https://dl.fedoraproject.org/pub/fedora/linux/releases/39/Everything/x86_64/os/Packages/r/rarpd-ss981107-63.fc39.x86_64.rpm>

Install the downloaded `RPM` file (replace file name if needed):

```shell
sudo rpm -i rarpd-ss981107-63.fc39.x86_64.rpm
```

## Configuring rarpd for FC7

Information on the FC7 needs to be added in two places.

In `/etc/ethers`:

```text
00:00:00:00:00:00 fc7-1
```

Replace `00:00:00:00:00:00` by your FC7 MAC address.

In `/etc/hosts`:

```text
192.168.1.80 fc7-1
```

When using more than one FC7, call e.g. the second one `fc7-2` and add its MAC
and IP addresses in the same way, increasing the IP number by 1, i.e. `192.168.1.81`.

## Enabling rarpd

Create the following file: `/etc/systemd/system/rarpd.service`
with the following content:

```ini
[Unit]
Description=Reverse Address Resolution Protocol Requests Server
Documentation=https://linux.die.net/man/8/rarpd
Requires=network.target
After=network.target

[Service]
Type=forking
User=root
#EnvironmentFile=/etc/sysconfig/rarpd
ExecStart=/usr/sbin/rarpd -e -v ethernet_card

[Install]
WantedBy=multi-user.target
```

Make sure to replace `ethernet_card` by the ethernet device connected to the FC7, e.g. `enp1s0`.

Enable and start the `rarpd` service:

```shell
sudo systemctl enable --now rarpd
```

You can watch the service activity using:

```shell
journalctl -fu rarpd.service
```
