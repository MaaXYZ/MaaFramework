FROM mcr.microsoft.com/devcontainers/cpp:1-ubuntu-22.04

COPY cmake.sh /tmp
COPY ninja-build.sh /tmp
COPY gcc-13_and_g++-13.sh /tmp

RUN sudo sh /tmp/cmake.sh && sudo sh /tmp/ninja-build.sh && sudo sh /tmp/gcc-13_and_g++-13.sh