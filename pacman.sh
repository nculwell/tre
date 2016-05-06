#!/bin/sh
MINGW=mingw64/mingw-w64-x86_64
PACKAGES="make $MINGW-make automake libtool autoconf $MINGW-cunit"
pacman -Syuu
pacman -Su --noconfirm $PACKAGES

