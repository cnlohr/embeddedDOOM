################################################################
#
# $Id:$
#
# $Log:$
#

HOSTGCC=gcc

CC=  gcc  # gcc or g++
CC_HOST = gcc
# 
CFLAGS:=-m32 -O1 -g -w -flto -DNORMALUNIX -DLINUX -DMAXPLAYERS=1 -DDISABLE_NETWORK  # -DUSEASM 
LDFLAGS:=-L/usr/X11R6/lib $(CFLAGS)
LIBS:=-lXext -lX11 -lnsl -lm -lpthread
CFLAGS_HOST:= $(CFLAGS)
LDFLAGS_HOST:=-L/usr/X11R6/lib
LIBS_HOST:=-lXext -lX11 -lnsl -lm -lpthread
CS:=i_video_console.c

#CFLAGS_FINAL:=$(CFLAGS)
#CFLAGS_FINAL+=-DFIXED_HEAP=262144
#CS:=i_video.c

# if you need to do low-level memory debugging.
CFLAGS_HOST+=-fsanitize=address -static-libasan -g

# For RISC-V
#CFLAGS:=-fno-stack-protector -static-libgcc -fdata-sections -ffunction-sections -g -Os -march=rv32ima -mabi=ilp32 -static -I/usr/include -DRV32IMA_BUILD -DDISABLE_NETWORK
#CFLAGS_HOST+=
#LDFLAGS:= -T flatfile-rv32ima.lds -nostdlib -Wl,--gc-sections
#LIBS:=
#SUPPORT_FILES:=startup.S
#CC:=riscv64-unknown-elf-gcc


BAKED:=support/rawwad.o support/baked_texture_data.o support/baked_map_data.o
# subdirectory for objects
O=linux

# not too sophisticated dependency
CS+=				\
		doomdef.c \
		doomstat.c \
		dstrings.c \
		i_system.c		\
		tables.c			\
		f_finale.c		\
		f_wipe.c 		\
		d_main.c			\
		d_items.c		\
		g_game.c			\
		m_menu.c			\
		m_misc.c			\
		m_argv.c  		\
		m_bbox.c			\
		m_fixed.c		\
		m_swap.c			\
		m_cheat.c		\
		m_random.c		\
		am_map.c			\
		p_ceilng.c		\
		p_doors.c		\
		p_enemy.c		\
		p_floor.c		\
		p_inter.c		\
		p_lights.c		\
		p_map.c			\
		p_maputl.c		\
		p_plats.c		\
		p_pspr.c			\
		p_setup.c		\
		p_sight.c		\
		p_spec.c			\
		p_switch.c		\
		p_mobj.c			\
		p_telept.c		\
		p_tick.c			\
		p_saveg.c		\
		p_user.c			\
		r_bsp.c			\
		r_data.c			\
		r_draw.c			\
		r_main.c			\
		r_plane.c		\
		r_segs.c			\
		r_sky.c			\
		r_things.c		\
		w_wad.c			\
		wi_stuff.c		\
		v_video.c		\
		st_lib.c			\
		st_stuff.c		\
		hu_stuff.c		\
		hu_lib.c			\
		z_zone.c			\
		info.c				\
		i_net.c			\
		d_net.c			\
		stubs.c    \
		sounds.c	\
		s_sound.c		\
		i_sound.c

all:	 emdoom

emdoom:	$(CS) i_main.c $(BAKED) $(SUPPORT_FILES)
	$(CC) $(CFLAGS_FINAL) $(LDFLAGS) $^ -o emdoom $(LIBS)

support/baked_texture_data.c : emdoom.gentables
	./emdoom.gentables

support/baked_map_data.c : emdoom.gentables
	./emdoom.gentables

support/baked_map_data.o : support/baked_map_data.c
	$(CC) $(CFLAGS) -c -o $@ $^

support/baked_texture_data.o : support/baked_texture_data.c
	$(CC) $(CFLAGS) -c -o $@ $^

support/wadder : support/wadder.c
	$(HOSTGCC) -o $@ $^

support/rawwad.c : support/wadder
	(cd support;./wadder;cd ..)

support/rawwad.o : support/rawwad.c
	$(CC) $(CFLAGS) -c -o tmp.o $^
	mv tmp.o $@

support/rawwad_host.o : support/rawwad.c
	$(CC_HOST) $(CFLAGS_HOST) -c -o tmp.o $^
	mv tmp.o $@

emdoom.size : emdoom
	nm --print-size --size-sort --radix=d emdoom  | grep -v " r " | grep -v " R "
	size emdoom -A

emdoom.gentables : i_main.c support/rawwad_host.o
	$(CC_HOST) $(CFLAGS_HOST) $(LDFLAGS_HOST) $^ -o emdoom.gentables -DGENERATE_BAKED $(CS) $(LIBS_HOST) 


clean:
	rm -f *.o *~ *.flc
	rm -f emdoom emdoom.gentables $(BAKED) support/wadder support/rawwad_host.o
	rm -rf support/baked_map_data.c  support/baked_texture_data.c

#############################################################
#
#############################################################
