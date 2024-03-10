# Kholors
Audio workstation without tracks and where all
imported samples are sharing the same on screen spectrum editor.
You can make track with it by importing samples, and save the
projects a git projects. See [this demo on YouTube](https://www.youtube.com/watch?v=TeF4ExiSIbU).

![](design/screenshot.png)

The use case is to make surgical mixdowns for electronic music. Only works on Linux. Discontinued.

## Building
Eventually install dependencies first (example is for ArchLinux).
```
sudo pacman -S gcc pkgconfig cmake make alsa-lib freetype2 webkit2gtk git clang yaml-cpp nlohmann-json libgit2 fftw
```

Then build and run.

```bash
mkdir build
cd build
cmake ..
make Kholors
```
# Running
```bash
# watch out, depending on if cmake is setup to include
# debugging build flags, you will get it here or
# in another Kholors_artefacts subfolder
./Kholors_artefacts/Debug/Kholors\ Example
```

## Testing
```bash
# if not already done, run cmake
cmake ..
make
make test
```
