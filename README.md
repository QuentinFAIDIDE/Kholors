# Kholors
Experimental audio software (alpha).

## Building
Eventually install dependencies first (example is for ArchLinux).
```
sudo pacman -S gcc pkgconfig cmake make alsa-lib freetype2 webkit2gtk git clang yaml-cpp
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
