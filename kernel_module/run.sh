sudo rmmod process_container
sudo make
sudo make install
sudo insmod process_container.ko
sudo chmod 777 /dev/pcontainer

sudo ../benchmark/benchmark 2 2 2
dmesg
