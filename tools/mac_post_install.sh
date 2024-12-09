#!/bin/bash

install_dir=$1
shift

if ! [[ -d "$install_dir" ]]; then
    echo "usage: $0 [maa framework install directory]"
    exit 1
fi

if ! [[ -e $install_dir/bin/libMaaFramework.dylib ]]; then
    echo "directory $install_dir doesn't have maa framework"
    exit 1
fi

function remove_and_sign_adhoc {
    if codesign -dv $1 2>&1 | grep "CodeDirectory" > /dev/null; then
        codesign --remove-signature $1
    fi
    codesign -s - $1
}

function fix_rpath() {
    if ! otool -L $install_dir/bin/libMaaFramework.dylib | grep libc++ > /dev/null; then
        echo "not linked to libc++, quit"
        exit 0
    fi

    libcxx_path=`otool -L $install_dir/bin/libMaaFramework.dylib | grep libc++ | awk '{print $1}'`

    echo "libc++.1.dylib: $libcxx_path"

    if [[ "$libcxx_path" == "@loader_path/libc++.1.dylib" ]]; then
        echo "already linked to @loader_path/libc++, quit"
        exit 0
    fi

    echo "change $libcxx_path to @loader_path/libc++.1.dylib"

    for lib in libc++ libc++abi libunwind; do
        rm $install_dir/bin/$lib.1.dylib
        rm $install_dir/bin/$lib.dylib
        mv $install_dir/bin/$lib.1.0.dylib $install_dir/bin/$lib.1.dylib

        bin=$install_dir/bin/$lib.1.dylib
        echo "processing $bin"
        if otool -L $bin | grep @rpath/libc++.1 > /dev/null; then
            install_name_tool -change @rpath/libc++.1.dylib @loader_path/libc++.1.dylib $bin
        fi
        if otool -L $bin | grep @rpath/libc++abi.1 > /dev/null; then
            install_name_tool -change @rpath/libc++abi.1.dylib @loader_path/libc++abi.1.dylib $bin
        fi
        if otool -L $bin | grep @rpath/libunwind.1 > /dev/null; then
            install_name_tool -change @rpath/libunwind.1.dylib @loader_path/libunwind.1.dylib $bin
        fi

        remove_and_sign_adhoc $bin
    done

    for bin in $install_dir/bin/*; do
        if [[ -f $bin ]]; then
            echo "processing $bin"
            install_name_tool -change $libcxx_path "@loader_path/libc++.1.dylib" $bin

            remove_and_sign_adhoc $bin
        fi
    done
}

function collect_symbols() {
    for lib in $install_dir/bin/*; do
        if [[ -f $lib ]] && basename $lib | grep -i maa > /dev/null; then
            echo "collect debug symbols of $lib"
            dsymutil $lib
        fi
    done
    rm -r $install_dir/symbol
    mkdir -p $install_dir/symbol
    mv $install_dir/bin/*.dSYM $install_dir/symbol/
}

while getopts "rd" opt; do
    case $opt in
        r)
            do_fix_rpath=true
            ;;
        d)
            do_collect_symbols=true
            ;;
    esac
done

if [[ "$do_fix_rpath" == "true" ]]; then
    fix_rpath
fi

if [[ "$do_collect_symbols" == "true" ]]; then
    collect_symbols
fi
