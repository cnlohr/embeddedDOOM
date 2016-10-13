################################################################
#
# $Id:$
#
# $Log:$
#

HOSTGCC=gcc

CC=  gcc  # gcc or g++

CFLAGS= -m32 -O0 -g -w -flto -DNORMALUNIX -DLINUX # -DUSEASM 
LDFLAGS=-L/usr/X11R6/lib $(CFLAGS)
LIBS=-lXext -lX11 -lnsl -lm -lpthread

BAKED:=support/rawwad.o support/baked_texture_data.o support/baked_map_data.o
# subdirectory for objects
O=linux

# not too sophisticated dependency
CS=				\
		doomdef.c \
		doomstat.c \
		dstrings.c \
		i_system.c		\
		i_video.c		\
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
		os_generic.c	\
		XDriver.c \
		DrawFunctions.c \
		i_net.c			\
		d_net.c			\
		stubs.c    \
		sounds.c	\
		s_sound.c		\
		i_sound.c

all:	 emdoom

emdoom:	$(CS) i_main.c $(BAKED)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o emdoom $(LIBS)

support/baked_texture_data.c : emdoom.gentables
	./emdoom.gentables

support/baked_map_data.c : emdoom.gentables
	./emdoom.gentables

support/wadder : support/wadder.c
	$(HOSTGCC) -o $@ $^

support/rawwad.c : support/wadder
	(cd support;./wadder;cd ..)

support/rawwad.o : support/rawwad.c
	$(CC) $(CFLAGS) -c -o tmp.o $^
	mv tmp.o $@

emdoom.size : emdoom
	nm --print-size --size-sort --radix=d emdoom  | grep -v " r " | grep -v " R "
	size emdoom -A

emdoom.gentables : $(CS) i_main.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o emdoom.gentables $(LIBS) support/rawwad.o -DGENERATE_BAKED


clean:
	rm -f *.o *~ *.flc
	rm -f emdoom emdoom.gentables $(BAKED) support/wadder


#############################################################
#
#############################################################
