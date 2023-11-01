dpkg -l | grep cmake > /dev/null 2>&1

if [ $? -eq 0 ]; then
    echo 'remove cmake'
    sudo apt -y remove --purge --auto-remove cmake
fi

sudo apt update && \
sudo apt install -y software-properties-common lsb-release && \
sudo apt clean all

wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null

sudo apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main" -y

sudo apt install cmake -y


