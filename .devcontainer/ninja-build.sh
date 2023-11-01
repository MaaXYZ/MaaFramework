dpkg -l | grep 'ninja-build' > /dev/null 2>&1

if [ $? -ne 0 ]; then
    echo 'install ninja'
    apt install ninja-build -y
fi 