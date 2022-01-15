# uploads picoboy image via SWD on a network attached rpi
scp build/picoboy.elf peta@10.0.0.38:/home/peta/delete/picoboy.elf
ssh peta@10.0.0.38 'openocd -f interface/raspberrypi-swd.cfg -f target/rp2040.cfg -c "program /home/peta/delete/picoboy.elf verify reset exit"' 
