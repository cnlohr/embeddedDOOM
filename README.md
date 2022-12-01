# embeddedDOOM

A port of DOOM targeted for memory-strapped systems.

The first rev of this is lifted directly from https://github.com/id-Software/DOOM.

This is specifically targeted for the shareware version of DOOM, and rips out a few features here and there in favor of being as small (RAM-wise) as possible.s

WARNING: This repository uses LFS to store the Shareware.  You will need to do a ```git lfs fetch``` in the firmware folder to get it updated.  Somtimes something goes wrong with it and you have to ```git lfs install --force;git lfs pull```

Right now, it uses (on -m32 / linux): 
 * .bss: 172,780 bytes
 * .bss:  + DOOM Heap (Some levels take more than others, E1M6 takes ~280kB, E1M1 takes ~107kB -- configurable in stubs.h -- variable is DOOMHeap)
 * .data: 45,600 bytes
 * .text: 123,858 bytes
 * .rodata: 5,690,440 bytes

## For installing on x64

```
sudo apt-get install gcc-multilib libx11-dev:i386
```

## Btw

Use ```./emdoom -warp 1 #``` where # is the map # (1 to 9)

I cut out screen transitions because it took an extra 64kB on the Heap!

## Files of interest

  * stubs.h/.c is where most of the control for the pairin
  * i_*.c has al the interface files for a host system.

## Building, etc.

  * Type "make" -- this builds the .wad into a .c, and into the target arch's .o.  Then, it builds doom with that and a very large heap.  It runs it once to generate all of the tables used for various things.  It then writes those tables to .c files in the support/ folder... Which it compiles into target arch .o's.  These .o's are linked in to provide the tables needed in .ro so they aren't taking up prescious ram.

To see memory usage:

 * Type "make emdoom.size" - it'll show you a list of all of the DATA, BSS and TEXT items, increasing in size (second column) as well as size of all sections.  You probably care about .text, .rodata, .data and .bss.

## TODO:

Figure out how to separate out things like the lines and line segments into rodata and .data/.bss.

## To port

Edit i_video.c
Edit i_system.c
Configure stubs.h to your liking.
