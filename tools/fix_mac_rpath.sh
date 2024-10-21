#!/bin/bash

install_dir=$1

if ! [[ -d "$install_dir" ]]; then
    echo "usage: $0 [maa framework install directory]"
    exit 1
fi

if ! [[ -e $install_dir/bin/libMaaFramework.dylib ]]; then
    echo "directory $install_dir doesn't have maa framework"
    exit 1
fi

if ! otool -L $install_dir/bin/libMaaFramework.dylib | grep libc++ > /dev/null; then
    echo "not linked to libc++, quit"
    exit 0
fi

libcxx_path=`otool -L $install_dir/bin/libMaaFramework.dylib | grep libc++ | awk '{print $1}'`

echo "libc++.1.dylib: $libcxx_path"

if [[ "$libcxx_path" == "@rpath/libc++.1.dylib" ]]; then
    echo "already linked to @rpath/libc++, quit"
    exit 0
fi

echo "change $libcxx_path to @rpath/libc++.1.dylib"

for bin in $install_dir/bin/*; do
    echo "processing $bin"
    install_name_tool -change $libcxx_path "@rpath/libc++.1.dylib" $bin
done
