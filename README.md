# embeddedDOOM

A port of DOOM targeted for memory-strapped systems.

The first rev of this is lifted directly from https://github.com/id-Software/DOOM.

This is specifically targeted for the shareware version of DOOM, and rips out a few features here and there in favor of being as small (RAM-wise) as possible.s

WARNING: This repository uses LFS to store the Shareware.  You will need to do a ```git lfs fetch``` in the firmware folder to get it updated.  Sometimes something goes wrong with it and you have to ```git lfs install --force; git lfs pull```

Right now, it uses (on -m32 / linux): 
 * .bss: 170,992 bytes + DOOM Heap (recommend 294,912 if possible, can use as little as 120kB for E1M1)
   * DOOM Heap: Some levels take more than others, E1M6 takes ~280kB, E1M1 takes ~107kB
   * Configurable in stubs.h -- variable is DOOMHeap
   * Framebuffer lives in this space.  It's `CombinedScreens` -- Note: This is not the normal behavior, normally DOOM Keeps 4 copies of each screen, but we just lie and alias all 4 copies.  It does mean you can't get the blood transitions between levels and the end screens get a tad sloppy.
 * .data: 45,600 bytes
 * .text: 108,536 bytes
 * .rodata: 5,685,632 bytes -- Includes the WAD files.

Also, this build requires no file I/O had has highly customizable user input and video output.

So, theoretically, you could run this on a system with only 384kB RAM.

## For installing on x64

Because DOOM (in its current state is 32-bit ONLY, you can only compile to 32-bit targets.  This is OK though, since the Makefile includes `-m32`, but you must install the 32-bit compat libs with the following:

#### On Debian/Ubuntu based systems:
```
sudo apt-get install gcc-multilib libx11-dev:i386 libxext-dev:i386 libnsl-dev:i386
```
#### On Arch based systems:
```
sudo pacman -S lib32-libx11 lib32-libxext lib32-libnsl gcc-multilib
```

## Btw

Use ```./emdoom -warp 1 2``` where second number is the map # (1 to 9)

I cut out screen transitions because it took an extra 64kB on the Heap!

## Files of interest

 * stubs.h/.c is where most of the control for the pairin
 * doomdef.h - just has some useful bits. Consider turning on `RANGECHECK` if you are going to be making substantial changes.
 * i_*.c has al the interface files for a host system.
 * Check out if the configuration of COMBINE_SCREENS is what you want.  It controls how the screen memory is allocated.


## Building, etc.

  * Type "make" -- this builds the .wad into a .c (if build step is uncommented) and into the target arch's .o.  Then, it builds doom with that and a very large heap.  It runs it once to generate all of the tables used for various things.  It then writes those tables to .c files in the support/ folder... Which it compiles into target arch .o's.  These .o's are linked in to provide the tables needed in .ro so they aren't taking up prescious ram.

To see memory usage:

 * Type "make emdoom.size" - it'll show you a list of all of the DATA, BSS and TEXT items, increasing in size (second column) as well as size of all sections.  You probably care about .text, .rodata, .data and .bss.

## TODO:

 * Figure out how to separate out things like the lines and line segments into rodata and .data/.bss.
 * Figure out how to change screen size successfully.  It just really loves making errors.

## To port

Edit i_video.c
Edit i_system.c
Configure stubs.h to your liking.
