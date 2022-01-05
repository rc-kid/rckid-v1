sudo mount /dev/sdb1 /mnt/pico
sudo cp build/picoboy.uf2 /mnt/pico
sudo sync
sudo umount /dev/sdb1
