FROM archlinux:latest

RUN pacman -Syu --noconfirm
RUN pacman -S --noconfirm gcc pkgconfig cmake make alsa-lib freetype2 webkit2gtk
RUN pacman -S --noconfirm git

WORKDIR /src/app