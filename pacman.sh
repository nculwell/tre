#!/bin/sh
PACKAGES='make mingw64/mingw-w64-x86_64-make automake libtool autoconf'
pacman -Syuu
pacman -Su --noconfirm $PACKAGES

