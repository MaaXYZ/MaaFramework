dpkg -l | grep cmake > /dev/null 2>&1

if [ $? -eq 0 ]; then
    echo 'remove cmake'
    apt -y remove --purge --auto-remove cmake
fi

apt update && \
apt install -y software-properties-common lsb-release && \
apt clean all

wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null

apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main" -y

apt install cmake -y


