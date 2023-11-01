dpkg -l | grep 'gcc-13' > /dev/null 2>&1

if [ $? -ne 0 ]; then
    echo 'install gcc-13 && g++-13'
    sudo apt install software-properties-common -y
    sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
    sudo apt update
    sudo apt install gcc-13 g++-13 -y
    sudo rm /usr/bin/gcc
    sudo rm /usr/bin/g++
    sudo ln -s /usr/bin/gcc-13 /usr/bin/gcc
    sudo ln -s /usr/bin/g++-13 /usr/bin/g++
fi


