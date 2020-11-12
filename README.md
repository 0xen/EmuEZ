### This project is under development and will see changes

# EmuEZ
EmuEZ is a work in progress system that encapsulated multiple custom made emulators into a single application

## Table of Contents
+ [Building](#Building)



## <a name="Building"></a> Building
### Libraries
#### Vulkan Rendering Libary
[Download/Clone](https://github.com/0xen/Vulkan) the vulkan libary in the same top level folder as EmuEZ before running CMake

### Windows
The project is created using CMake and will allow you to build the example executables or as a Visual Studio Solution

#### Build Visual Studio Solution
Run the following command line or 'build_vs_solution.bat' to build visual studio 2019
```cmake -H. -Bbuild --log-level=NOTICE -G "Visual Studio 16 2019" -A x64```

## Emulator Compatibility
### Gameboy
Test Package | Test | Result
------------ | ------------- | -------------
Blargs | cpu_instrs | :heavy_check_mark:
Blargs | instr_timing | :heavy_check_mark:
Blargs | mem_timing | :heavy_check_mark:
Blargs | mem_timing-2 | :heavy_check_mark:
Blargs | cgb_sound | :x:
Blargs | dmg_sound | :x:
Blargs | interrupt_time | :x:
Blargs | oam_bug | :x:
Blargs | halt_bug | :x:
