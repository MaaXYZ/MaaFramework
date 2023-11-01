dpkg -l | grep 'gcc-13' > /dev/null 2>&1

if [ $? -ne 0 ]; then
    echo 'install gcc-13 && g++-13'
    apt install software-properties-common -y
    add-apt-repository ppa:ubuntu-toolchain-r/test -y
    apt update
    apt install gcc-13 g++-13 -y
    rm /usr/bin/gcc
    rm /usr/bin/g++
    ln -s /usr/bin/gcc-13 /usr/bin/gcc
    ln -s /usr/bin/g++-13 /usr/bin/g++
fi


