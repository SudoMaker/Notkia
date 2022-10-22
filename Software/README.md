# Software

## Usage
### Linux kernel
- Download latest 5.19.x Linux kernel and apply the HUGE patch in **kernel** directory
- Install MIPS toolchain, e.g. `apt install build-essential gcc-mipsel-linux-gnu`
- Build the kernel image: `make ARCH=mips CROSS_COMPILE=mipsel-linux-gnu- -j8 uImage`

### rootfs
- Download Buildroot 2022.02.2 and use the config file in **buildroot** directory.
- Copy everything in the **rootfs-overlay** directory to the resulting rootfs.
- Create the filesystem image using `mkfs.jffs2 -e 0x10000 --with-xattr -p -l`

### U-Boot, download tool and chip datasheets
- Please visit [Ingenic Community](https://github.com/Ingenic-community).

### User Applications
- See **sources** directory.

## Credits
### Demo songs
**Palmtree.opus** - (C) SEGA, 1992. Downloaded from [here](https://www.youtube.com/watch?v=eQzrfwBFwQA)

**Snowy.ogg** - Author: Chris Leutwyler

## Notes
If you have any questions, feel free to ask on our Twitter / Discord.
