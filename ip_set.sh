sudo ip addr flush dev eth0
sudo ip addr add 169.254.100.1/16 dev eth0
sudo ip link set eth0 up