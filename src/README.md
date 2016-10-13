NOTES:

Updated stubs.h
Cleared out wipes.
Edit i_video.c
Edit i_system.c
Screens 0..3 are combined into the same memory, wacks out border a little bit in gameplay and causes smearing on end screen (Also, no transitions!)



TODO:
 * Bake Composite Textures (See r_data.c) (Mostly done)
 * Bake P_SETUP stuff:
	MALLOC: 17600 50 (nil) @ p_setup.c:239 -> Free: 0xf735d2d0 / 0xf735c4a4
	MALLOC: 26460 50 (nil) @ p_setup.c:447 -> Free: 0xf73652e8 / 0xf736097c
	MALLOC: 66112 50 (nil) @ p_setup.c:368 -> 0xf7331de0
	MALLOC: 3584 50 (nil) @ p_setup.c:210 -> 0xf7342038
	MALLOC: 23244 50 (nil) @ p_setup.c:275 -> 0xf7342e50
	MALLOC: 46816 50 (nil) @ p_setup.c:170 -> Free: 0xf734ffe8 / 0xf7348934
	MALLOC: 5288 50 (nil) @ p_setup.c:537 -> Free: 0xf7354404
	Free: 0xf735541c
	0xf735402c

 * Save 16k, precompute: viewangletox

TODO: Bake ylookup, columnofs


VVVV These totes can be done.
lighttable_t*		scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
lighttable_t*		scalelightfixed[MAXLIGHTSCALE];
lighttable_t*		zlight[LIGHTLEVELS][MAXLIGHTZ];

