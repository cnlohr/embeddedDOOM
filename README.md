# embeddedDOOM

A port of DOOM targeted for memory-strapped systems.

The first rev of this is lifted directly from https://github.com/id-Software/DOOM.

This is specifically targeted for the shareware version of DOOM, and rips out a few features here and there in favor of being as small (RAM-wise) as possible.s

WARNING: This repository uses LFS to store the Shareware.  You will need to do a ```git lfs fetch``` in the firmware folder to get it updated.

TODO: Write more here!



## Files of interest

  * stubs.h/.c is where most of the control for the pairin
  * i_*.c has al the interface files for a host system.

## Building, etc.

  * Type "make"

To see memory usage:

 * Type "make emdoom.size" - it'll show you a list of all of the DATA, BSS and TEXT items, increasing in size (second column) as well as size of all sections.  You probably care about .text, .rodata, .data and .bss.

