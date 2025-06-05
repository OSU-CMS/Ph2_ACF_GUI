# Installing additionally required software

## Libraries needed by Ph2_ACF

```shell
sudo dnf install -y boost-devel pugixml-devel json-devel
```

## Erlang (needed by uHAL)

Create a new file `/etc/yum.repos.d/modern_erlang.repo` with the following content, see [github.com/rabbitmq/erlang-rpm/blob/main/README.md](https://github.com/rabbitmq/erlang-rpm/blob/main/README.md) for more details:

```text
[modern-erlang]
name=modern-erlang-el9
# uses a Cloudsmith mirror @ yum1.novemberain.com.
# Unlike Cloudsmith, it does not have traffic quotas
baseurl=https://yum1.novemberain.com/erlang/el/9/$basearch
repo_gpgcheck=1
enabled=1
gpgkey=https://dl.cloudsmith.io/public/rabbitmq/rabbitmq-erlang/gpg.E495BB49CC4BBE5B.key
gpgcheck=1
sslverify=1
sslcacert=/etc/pki/tls/certs/ca-bundle.crt
metadata_expire=300
pkg_gpgcheck=1
autorefresh=1
type=rpm-md

[modern-erlang-noarch]
name=modern-erlang-el9-noarch
# uses a Cloudsmith mirror @ yum1.novemberain.com.
# Unlike Cloudsmith, it does not have traffic quotas
baseurl=https://yum1.novemberain.com/erlang/el/9/noarch
repo_gpgcheck=1
enabled=1
gpgkey=https://dl.cloudsmith.io/public/rabbitmq/rabbitmq-erlang/gpg.E495BB49CC4BBE5B.key
        https://github.com/rabbitmq/signing-keys/releases/download/2.0/rabbitmq-release-signing-key.asc
gpgcheck=1
sslverify=1
sslcacert=/etc/pki/tls/certs/ca-bundle.crt
metadata_expire=300
pkg_gpgcheck=1
autorefresh=1
type=rpm-md

[modern-erlang-source]
name=modern-erlang-el9-source
# uses a Cloudsmith mirror @ yum1.novemberain.com.
# Unlike Cloudsmith, it does not have traffic quotas
baseurl=https://yum1.novemberain.com/erlang/el/9/SRPMS
repo_gpgcheck=1
enabled=1
gpgkey=https://dl.cloudsmith.io/public/rabbitmq/rabbitmq-erlang/gpg.E495BB49CC4BBE5B.key
        https://github.com/rabbitmq/signing-keys/releases/download/2.0/rabbitmq-release-signing-key.asc
gpgcheck=1
sslverify=1
sslcacert=/etc/pki/tls/certs/ca-bundle.crt
metadata_expire=300
pkg_gpgcheck=1
autorefresh=1
```

Then run the following commands:

```shell
sudo dnf update
sudo dnf install erlang
```

## uHAL libraries (cactus)

```shell
sudo curl https://ipbus.web.cern.ch/doc/user/html/_downloads/ipbus-sw.repo -o /etc/yum.repos.d/ipbus-sw.repo
sudo dnf clean all
sudo dnf groupinstall -y uhal controlhub
```

## ROOT

```shell
sudo dnf install -y root root-net-http root-net-httpsniff root-graf3d-gl \
  root-physics root-montecarlo-eg root-graf3d-eve root-geom libusb-devel \
  xorg-x11-xauth.x86_64
```

## Build tools and some nice git extras

```shell
sudo dnf install -y cmake3 clang-tools-extra git-extras
```

## gcc-toolset-12

```shell
sudo dnf makecache --refresh
sudo dnf -y install gcc-toolset-12
```

## python3

```shell
sudo dnf install -y python3 python3-devel
```

## protobuf

Follow instructions to install protobuf from [gitlab.cern.ch/cms_tk_ph2/MessageUtils/-/blob/master/README.md](https://gitlab.cern.ch/cms_tk_ph2/MessageUtils/-/blob/master/README.md) (only the install part is needed)

## pybind11

If installed in parallel to the directory where you plan to install `Ph2_ACF`, the `setup.sh` script will point to the correct location.

```shell
wget https://github.com/pybind/pybind11/archive/refs/tags/v2.9.2.tar.gz
tar zxvf v2.9.2.tar.gz
```

## Git LFS

```shell
sudo dnf install git-lfs
git lfs install
```
