#!/bin/bash

# Exit on error
set -e

# ANSI Escape Codes
GRN='\033[0;32m'
YLW='\033[0;33m'
RED='\033[0;31m'
BLU='\033[0;34m'
RST='\033[0m'

# Check if a given command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

build_process() {
    # Remove build dirs if they already exist
    for dir in SVT-AV1 aom dav1d libgav1 libjpeg-turbo libwebp libxml2 libyuv zlib libpng; do
        if [ -d "ext/$dir" ]; then
            echo -e "${YLW}Cleanup existing $dir ...${RST}"
            rm -rf ext/$dir
        fi
    done

    # Mark the scripts as executable
    chmod +x ext/*.{sh,cmd}

    echo -e "${YLW}Configuring libavif & dependencies...${RST}"
    cd ext || exit 1
    echo -e "\nConfiguring libxml2..." && bash libxml2.cmd
    echo -e "\nConfiguring libargparse..." && bash libargparse.cmd
    echo -e "\nConfiguring zlibpng..." && bash zlibpng.cmd
    echo -e "\nConfiguring libyuv..." && bash libyuv.cmd
    echo -e "\nConfiguring libsharpyuv..." && bash libsharpyuv.cmd
    echo -e "\nConfiguring libjpeg..." && bash libjpeg.cmd
    echo -e "\nConfiguring SVT-AV1-PSY..." && bash svt.sh
    echo -e "\nConfiguring aom-psy101..." && bash aom.cmd
    echo -e "\nConfiguring dav1d..." && bash dav1d.cmd
    echo -e "\nConfiguring libgav1..." && bash libgav1.cmd
    cd ..
    echo -e "${BLU}Configuration process complete${RST}"
    cmake_opts=(
        -DAVIF_CODEC_AOM=LOCAL
        -DAVIF_CODEC_SVT=LOCAL
        -DAVIF_CODEC_DAV1D=LOCAL
        -DAVIF_CODEC_LIBGAV1=LOCAL
        -DAVIF_LIBYUV=LOCAL
        -DAVIF_LIBSHARPYUV=LOCAL
        -DAVIF_JPEG=LOCAL
        -DAVIF_ZLIBPNG=LOCAL
        -DAVIF_BUILD_APPS=ON
        -DAVIF_LIBXML2=LOCAL
        -DAVIF_ENABLE_EXPERIMENTAL_GAIN_MAP=ON
        -DAVIF_ENABLE_EXPERIMENTAL_JPEG_GAIN_MAP_CONVERSION=ON
    )
    echo -e "\nConfiguring libavif..."
    cmake -S . -B build -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX="${installpfx}" "${cmake_opts[@]}" -Wno-dev
    echo -e "\nCompiling libavif..."
    cmake --build build --parallel
    echo -e "${GRN}Compilation process complete${RST}"

    # Cleanup build dirs
    for dir in SVT-AV1 aom dav1d libgav1 libjpeg-turbo libwebp libxml2 libyuv zlib libpng; do
        if [ -d "ext/$dir" ]; then
            rm -rf ext/$dir
        fi
    done
}

main() {
    # Check for dependencies
    for cmd in git cmake ninja meson clang nasm; do
        echo -ne "$cmd\t"
        if ! command_exists $cmd; then
            echo -e "${RED}X\nError: $cmd is not installed. Please install it & try again.${RST}"
            exit 1
        else
            echo -e "${GRN}✔${RST}"
        fi
    done

    # Begin build process
    installpfx="/usr/local"
    build_process

    # Install the binary
    sudo cmake --install build --config Release --strip --verbose
    strip build/avifgainmaputil && sudo cp build/avifgainmaputil ${installpfx}/bin/
    sudo cp oavif.sh ${installpfx}/bin/oavif
    LD_LIBRARY_PATH=${installpfx}/lib ${installpfx}/bin/avifenc --version 2>&1
    echo -e "${GRN}avifenc & oavif have been installed to ${installpfx}/bin/${RST}"
}

main

