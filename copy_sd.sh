sh fast_build.sh
cp -a raspi-base/start.elf raspi-base/fixup.dat raspi-base/bcm2710-rpi-zero-2-w.dtb kernel8.img config.txt /Volumes/BOOT
cp -a raspi-base/overlays /Volumes/BOOT/overlays
cp -a raspi-base/bootcode.bin /Volumes/BOOT
(cd /Volumes/BOOT; chmod 644 *; chmod -R 755 overlays)
