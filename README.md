# Kholors
Experimental audio software (alpha).

## Building
Eventually install dependencies first.
```
sudo pacman -S gcc pkgconfig cmake make
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
