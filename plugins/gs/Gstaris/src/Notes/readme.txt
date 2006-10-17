


_________________________________
PS2 emulators GS plugin - GStaris
¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
Author: Absolute0 - absolutezero@ifrance.com
Date:   january,8 2003



_____________________________________________________________________________
 "I only want to take an hybrid rainbow in order to have more crazy sunshine
  in my life in this furikuri world."

                                                                   Absolute0
¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯



Support
¯¯¯¯¯¯¯
Please support GStaris by making a donation. You can donate money
using PayPal (www.paypal.com). Send donations to absolutezero@ifrance.com.
If you want to make other kind of donations (hardware, games...), please contact me.



Contents:
¯¯¯¯¯¯¯¯¯
  * Some words from the autor              my words :)
  * Thanks                                 the people i would like to thank
  * Files                                  the files in this .zip
  * Installation                           how to install it
  * Configuration                          how to configure it
  * Key description                        control the plugin using these keys
  * Version                                last things added
  * Bugs                                   known bugs



Some words from the autor:
¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
  What's new ?
	ALL

  Maybe some demos will work better using 'Interpreter CPU' and 'Disable VSync speed hack' with P©SX2
  Please report me bugs, speed and other comments. Don't send me any files without asking before.

  And about some questions:
    The plugin doesn't run ps2 games !
    OpenGL is the best API for nVidia cards :) - but it works with the others cards too ! :).
    Which demo works ? Just test them :).
	You want to make your plugin? Ok, contact shadow or linuzappz, and if you need help email me.
	When will the next GStaris be released ? When i'll be happy and when it will be finished.
    For some others questions, please send an email at < absolutezero@ifrance.com >.



Thanks:
¯¯¯¯¯¯¯
  * The P©SX2 team (hi shadow ! hi linuzappz !)
  * The PS2PE team (hi Goldfinger !)
  * Antoche, and the #codefr team (IRCnet) (youhou starman !)
  * saquib, asad
  * my friends for their moral support: LIA, ryô, morphin, and klang
  * majestic3 for his Taris logo
  * A big thanks to the ps2 developpers, particulary for all the authors of the demos i have :)
  * NickK,Pete,...
  * betatesters jegHegy, bositman
  * The others, and so many more...



Files:
¯¯¯¯¯¯
  * README.txt      this file !
  * GStaris.dll     the release of the GStaris OpenGL plugin



Installation:
¯¯¯¯¯¯¯¯¯¯¯¯¯
 Copy the .dll files to the plugin directory of the emulator.
 If you have some problems, please consult the documentation of the emulator or this file & send me some details.
 You can also visit the P©SX2 support site by the PsxFanatics team at: http://pcsx2.psxfanatics.com



Configuration:
¯¯¯¯¯¯¯¯¯¯¯¯¯¯
 * Resolution:
      Choose the display resolution. Some stuff may not work with resolution != 640x480.
  
 * Fullscreen mode:
      The speed may be a little better in fullscreen mode.
 
 * Stretch:
      Stretch the render.

 * Keep ps2 ratio:
      The ratio of the ps2 will be used in stretched mode.
  
 * Force 75 Hz [Fullscreen]:
      Use 75 Hz in fullscreen mode instead of the default frequency.
  
 * Use FPS limit:
      Check it if you want to use FPS limit. - but it's not writed in the plugin yet ! ^-^
  
 * Show status:
      Display the menu (fps count, options, ...). It can be displayed also using Insert.
  
 * Disable alpha blending:
      Enable alpha blending for all the demos ; result depends on the demos
  
 * Antiflickering-fix:
      You may always disable the FB Fix (no fb fix), but if you want to disable flickering with some demos, you can enable the fb fix 1 or 2.

 * Force antialiasing:
 * Force linear texturing:
      Better viewing in some demos, try to render a little smoother when it's not already required by the demos
  
 * Missing objects fix:
      Enable it in order to see some missing objects in pillgen,1fx, and maybe others demos
  
 * Disable IDT onscreen:
      Will disable a function in order to have better results with NoRecess demos.



Key description:
¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
Insert:    show/hide menu    (fps count)
Suppr:     fbfix change
Page Down: menu change
Page Up:   menu change
Home:      enable/disable logging
End:       texture viewer menu change

F8:        make a screenshot of screen and VRAM (snap/snap_.bmp)

F11:       texture viewer ^ tView - 1;
F12:       texture viewer ^ tView + 1;



Versions:
¯¯¯¯¯¯¯¯¯
        VERSION 0.66
        ------------
-Up to the latest pcsx2 specifications .NO savestate support! (shadow)

 
        VERSION 0.65
        ------------
-Up to pcsx2 0.4 specifications by shadow of pcsx2 team :))

	VERSION 0.6
	¯¯¯¯¯¯¯¯¯¯¯
- Now plugin is OpenSource
- Plugin was rewrited so all is new :)

    VERSION 0.5
    ¯¯¯¯¯¯¯¯¯¯¯
- Fixed wrap bug in plasma_tunnel & soundcheck tunnel
- Added FBfix5 and FBfix6 : use them for colors15,psms,carshowroom,plasma_tunnel,...- all the demos :P
- ...
todo: - Fix flatline demo (display register bug)
todo: texture func

    VERSION 0.4
    ¯¯¯¯¯¯¯¯¯¯¯
- Fixed [PSMT8*CSM1*PSMCT32] texture format (ps2mame title !)
- GSprocessTexture rewritten, should be more compatible.
- Added mipmapping support
- New menu, without flickering :p
- Added Texture Viewer in menu
- Now the cursor will be available only in windowed mode.
- Fixed keep ps2 ratio mode, fixed screen centered bugs, etc...
- Texture format added: [PSMT4*CSM1*PSMCT16]  [PSMT4*CSM1*PSMCT16S] [PSMT4*CSM2*PSMCT16]
                        [PSMT8H*CSM1*PSMCT32] [PSMT8H*CSM1*PSMCT16] [PSMT8H*CSM1*PSMCT16S] [PSMT8H*CSM2*PSMCT16]
  I still need some demos to test...
- Sprite code correction (psms,ps2mame,others) ; now it's like the GSsoft plugin.
- A lot of optimisations - speed is really better (5-10 fps with some demos).
  But the following additions will slow down the emulation of some demos :p
- New updateTexture checking code (psms games works! - soundcheck text colors works! - colors15 textures begin to work)
- new drawing code (for Point,Line,Triangle,Sprite) - faster, more compatible
- fixed 1fx alpha blending
- fixed PRIM register bug (plasma_tunnel)
- Added 'Missing stuff Fix' for 1fx,pillgen and maybe other demos
- Added FBfix 3&4, now FBfix is displayed in main menu, now you can change FBfix dynamically using 'End' button
- Fixed a bug when shutdown emulator without ESCAPE button (thanks Seta-San)
- Internal changes and code cleaning.

    VERSION 0.3 - Ryô's birthday
    ¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
- New z calculation (thanks saquib, for giving me the beginning of my actual code)
- Fixed jasper
- GIF correction
- 'About dialog' changes
- [PSMT8*CSM1*PSMCT16] & [PSMT8*CSM1*PSMCT16S] texture format added - need a demo to test :)
- pillgen;psms;soundcheck;1fx correction > Frame Buffer Fix 1
- 3starsII                               > Frame Buffer Fix 2
- alpha blending correction - now alpha blending is enabled by default
- Added experimental 'Keep ps2 ratio' (report me your comments)
- Now in non-stretched mode, screen will be centered (report me your comments)
- New scissoring code, with hardware&software support (a little part is missing)
- TEX1 register partially added
- STQ correction - works better, not perfectly
- New sprite clipping code (bytheway,demo2d,...)
- Added RGB24 -> RGB32 conversion in IDTwrite (3starsII title,...)
- Added RBG16 -> RGB32 conversion in IDTwrite (soundcheck tunnel,jum1,...)
- Compatible with the old and the new GS defs
- Fixed crash when using alpha blending code on old video cards (thanks asad)
- Fixed the bitbltbuf.dbw==0 bug (not the asad's fix, but another based on his fix)
- [PSMT4*CSM1*PSMCT32] texture format fixed (soundcheck & ps2mame text ; [i said bytheway in 0.2 changelog, it's an error sorry])
- Internal changes.

    VERSION 0.2
    ¯¯¯¯¯¯¯¯¯¯¯
- Bug correction with the textured sprite primitives (turnip)
- Added a texture format used by 'bytheway' (if i remember well) & 'soundcheck' [PSMT4*CSM1*PSMCT32]
- STQ texture coordinate added
- Correction of a silly bug in the XYZ__ registers (hi shadow !)
- Added 'Display Fps Count' to display the menu (can be displayed also using Inser)
- Added 'Polygon Mode' to choose the polygon draw mode: fill,line,point - for debugging
- Added 'Stretch Mode' you know for what :)
- Depth test (z) bug correction (3stars,...)
- Fixed fullscreen mode
- Added partial texture function support
- Added a texture format used by ps2mame [PSMT8*CSM1*PSMCT32] - bugged :)
- GSprocessTexture function correction (ps2mame) - (hi shadow !)
- IDT (Image Data Transmission) correction - space, turnip (init text)
- Added 'Resolution' to choose the resolution
- Snapshot code correction
- Added 'Force 75 Hz [Fullscreen]' - can be useful
- Fixed drawSpriteT func (boredom)
- New RGBA code - compatibility is better
- VERY partial alpha blending code
- Added depth test in sprite drawing (drawSpriteZ ; drawSpriteTZ)
- New primitive drawing function selection - faster with some demos
- Complete alpha blending code (buggy)
- Added 'Enable alpha blending' to choose beetween the very partial code and the complete code
- And many and many little corrections and optimizations :)...

    VERSION 0.1 - First release
    ¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
  * version 0.1 part B
- Bug with the *printf & the beginning of jasper corrected. printf, newprintf runs.
- TEX0 register correction (tmapped).
- Primitive drawing correction. tmapped run.
- Primitive drawing second correction. psms & psms-naplink primitives runs.
- Added flat & gouraud shading
- Added support for 8bit textures using the CLUT (psms). psms texture runs but i can't test all now.
- sprite primitive drawing correction
- prim funcs completely rewrited
- Depth test without speed decrease
- Some bugs with psms were corrected just before releasing the plugin :)
- Added some display correction (psms,space - maybe others i don't have)
- tested with PS2PE
- texture selection bug correction - space and others will work with the rights textures
- little primitive drawing correction > speed is up with many demos (fire,space,...)
- added texture update (when needed) ; speed is a little low but compatibility is really better (space,turnip,...)

  * version 0.1 part A
- OpenGL hardware rendering
- point,line,triangle,sprite primitives supported
- can run many demos with high speed
- experimental fullscreen mode 640*480*32 only
- windowed mode 640*480 with bpp >= 16 (i can't test with 15)
- option: fps limit at 60 fps ; choose fps limit value

  * First BETA version
- software image data processing
- can run some demo, but it's slow
- tested with P©SX2


