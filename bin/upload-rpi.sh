# uploads picoboy image via SWD on a network attached rpi
scp build/picoboy.uf2 peta@10.0.0.38:/home/peta/delete/picoboy.uf2
ssh peta@10.0.0.38 '~/.bin/pico-program'
ssh peta@10.0.0.38 'sudo mount /dev/sda1 /mnt/pico ; sudo cp /home/peta/delete/picoboy.uf2 /mnt/pico ; sudo sync ; sudo umount /dev/sda1' 
