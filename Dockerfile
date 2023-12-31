FROM archlinux:latest

RUN pacman -Syu --noconfirm
RUN pacman -S --noconfirm gcc pkgconfig cmake make alsa-lib freetype2 webkit2gtk
RUN pacman -S --noconfirm git
RUN pacman -S --noconfirm clang
RUN pacman -S --noconfirm yaml-cpp
RUN pacman -S --noconfirm nlohmann-json
RUN pacman -S --noconfirm ccache
RUN pacman -S --noconfirm libgit2
RUN pacman -S --noconfirm fftw

WORKDIR /src/app