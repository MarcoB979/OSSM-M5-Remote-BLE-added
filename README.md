# Project OSSM M5 REMOTE
## Overview of the OSSM-M5-Remote

A Remote Control Platform for with a focus on the [OSSM](https://github.com/KinkyMakers/OSSM-hardware) and other related ESP Controlled Sex Toys (like Eject cumpump and Fist-IT).

![Final Addon](image/remote.png?raw=true "Remote" )

Intially developed for the OSSM Project: 
https://github.com/KinkyMakers/OSSM-hardware

To help with development and design join the KinkyMakers Discord: https://discord.gg/MmpT9xE . Be sure to say hello in the #m5-remote channel. 

## M5 Remote now works with the official OSSM BLE firmware. ##
If you still have the 'old' ESP firmware (used for previous M5 remote firmwares), then this Branch is needed: https://github.com/ortlof/OSSM-Stroke

The Cable remote is obsolete with this firmware. If you want to go back you need to flash the OSSM orginal firmware back.

## Supported in this version of the M5 remote:
| OSSM Machine | https://github.com/KinkyMakers/OSSM-hardware |

## Additional toy support:

| EJECT | https://github.com/MarcoB979/EJECT-Cumpump | [A Work in Progress]
| FIST-IT | https://github.com/MarcoB979/Fist-IT | [A Work in Progress]

For information on how to build and source your materials, see below or see the official M5 remote page here:
https://github.com/ortlof/OSSM-M5-Remote

## Operation, functionality and screens:
Steps to initialize start-up:
1. Power on the OSSM and let it home.
2. Power on the OSSM M5 Remote.
3. M5 remote will try an auto connect first. If the OSSM was not ready and connection is unsuccesfull you can try an other time by pressing the left encoder to select 'Connect' and try to connect manually.
4. After successfull connection, the menu screen will show.

## Menu screen:
![Final Addon](image/Menu.jpg?raw=true "Menu" )
in the menu screen you can select the various options and go to the respective screens:
- Home (OSSM Control)
- Bator Mode (OSSM control for strokers)
- Settings (to change the different settings)
- Addons (to open and/or activate (show/hide) the available addons.
    - Available addons are now: Eject, Fist-IT and Streaming mode

* make your selection by rotating the right encoder.

* Pressing the left encoder button makes the M5 remote restart (after you confirmed the notification)
* Pressing the middle (MX/square) button, you can change the colors used in the screen (ui themes are built in)
* Pressing the right encoder button selects the actively selected menu option

## (OSSM) Home screen
![Final Addon](image/OSSM-home.jpg?raw=true "Home" )
This is the home screen where you can control your OSSM after successfull connection. the OSSM is controlled in 'Stroke Engine' mode.
By rotating the left encoder you control speed
By rotating the second encoder you change depth
By rotating the third encoder you change stroke (notice the slider shows the stroke depth relative to the depth)
By rotating the fourth (right) encoder you can change sensation (influences pattern behaviour)

Rotating an encoder to the right increases (+) the values, to the left the values decreases (-) the values. If in settings you selected to invert the stroke, rotating the slider increases when rotating to the right, if unchecked the values will decrease.

If stroke, speed and depth are positive values the OSSM will start automatically. if one of the values is set to 0, the OSSM will stop.

You can also press the middle button to start and stop.

When long pressing the middle button, you activate the emergency stop: the OSSM will slowly retract itself and go back to the menu screen. The OSSM MUST be re-homed before you can start to play again.

When pressing the left button, the OSSM will only do the pullout movement as described, but not trigger an emergency stop or go back to the menu.

To change a pattern, click the right button to goto the Pattern selection screen.

## Bator Mode
![Final Addon](image/Bator-mode.jpg?raw=true "Bator" )
Bator mode has the same functionality as the OSSM ho.e screen. However, the OSSM movement is now suitable for strokers. the OSSM will now move with the middle of the rail as starting point. Only speed, stroke and sensation can be set. When stroke is changed, the OSSM will move from middle to the size of stroke. If stroke is set to 20% as example, the OSSM will move from middle of the rail (50%) to 10 %on the left (40%) and then to 60% of the rail lengte (making it a 20% stroke move)

Clicking the right button brings you in the pattern selection menu. Clicking the left encoder return to the menu screen.
The middle square button has the same start/stop behaviour as the OSSM home screen, including the long press emergency stop.

## Settings
![Final Addon](image/Settings.jpg?raw=true "Settings" )
In settings you can change the following settings:
Vibrate: should the M5 Remote vibrate after button presses to give haptic feedback. 
Touch Disabled: if you do not want to be able to use the touchscreen, then check this option.
Stroke inverted: change the bravoure of the encoder where you change the stroke values in the OSSM Ho.e screen (rotating right games stoke go up, or down.
Force re-home: if we switch to the Menu screen and then back to screens where you can control the OSSM, this can force a homeing procedure. This is a safety measure, but sometimes this might be not totally necessary if you use the 57AIM Gold Motor. In that case you can choose to disable to force re-homes on occasions where this is not absolutely necessary. PLEASE NOTE: it is recommended to have this selected. EXTRA CAUTION: if you own the IHSV57 motor, you MUST keep this option activated for safety reasons!

You can also change the screen brightness to fit your needs and save on battery life. To do this, rotate the 3rd rotary encoder (seen from the left). Changes are applied immediately.

By pressing the left encoder, you save the settings in memory. The M5 Remote will use these saved settings at next start-up.
Rotating the right encoder will scroll through the options. Pressing it will select or de-select the active option

By pressing the middle button, you return to the menu screen. Do not forget to save first.

## Addons
![Final Addon](image/Addons.jpg?raw=true "Addons" )
# Addons - Streaming mode
![Final Addon](image/Streaming.jpg?raw=true "Streaming" )
# Addons - EJECT cumpump
![Final Addon](image/Fist-IT.jpg?raw=true "Eject" )
# Addons - Fist-IT
![Final Addon](image/Eject.jpg?raw=true "Fist-IT" )







## Operation, or how do I use it?

1. Power on the OSSM and let it home.
2. Power on the OSSM M5 Remote.
3. Press the left encoder to select 'Connect'
4. You can verify it is connected by looking in the top left corner, it should say 'connected'.
5. You can now begin use. You'll need to set the speed, depth and stroke to more than 0 and press the middle key to start. Start the speed out slow. 





# Build the OSSM M5 Remote Yourself

### Assembly instructions: [Klick Here !](Assembly.md)

## Bill Of Materials for sourcing Electrical Components

All M5Stack Core2 and CoreS3 are supported Now.

BOM is on Octopart for Easy Sourcing: https://octopart.com/bom-tool/rURYMuwB

PCB Files are located in the /OSSM-M5-Remote/Hardware/PCB folder if you want to make one yourself or use a different manufacturer other than PCBWay.

## Additional parts needed that are not PCB:  

| Quantity | Part | Sourcing EU | Price € |
|----------|------|-------------|---------|
| 1x | M5Stack CoreS3 SE | https://www.digikey.de/de/products/detail/m5stack-technology-co-ltd/K128-SE/23628221?s=N4IgTCBcDaILIFYDKAXAhgYwNYAIDCA9gE4CmSAzDkgKIgC6AvkA | 43 € |
| 2x | M3x25mm Hex Head Cap Bolt | https://www.amazon.de/Edelstahl-Innensechskant-Bolzenset-Eisenrahmen-Mechanischer-Innensechskantschraube-Mutternset/dp/B07PPFT871/ | 12,97 € |
| 4x | M3x20mm Hex Head Cap Bolt | Comes as part of the set mentioned above | " | 
| 4x | Heat Set inserts M3 | https://www.amazon.de/ruthex-Gewindeeinsatz-St%C3%BCck-Gewindebuchsen-Kunststoffteile/dp/B08BCRZZS3 | 8,99 € |
| 1x | 3,7v 2000mAh Lipo Batterie Size 34,5 mm x 10,6 mm x 56 mm | https://www.amazon.de/EEMB-103454-2AhLithium-Schutzplatine-Isolationsbeschichtung/dp/B08214DJLJ/ | 14,89 € |
| 4x | Encoder Knob Bought or 3D Printed | https://de.aliexpress.com/item/1005001394286414.html | 5 € |
| 1x | OSSM M5 Remote PCB | KinyMaker Discord #M5-Remote Channel or https://www.pcbway.com/project/shareproject/M5Stack_Core2_Remote_Plattform_2cb5bac0.html | 15 € |

--------------------------------------------

| Quantity | Part | Sourcing US | Price $ |
|----------|------|-------------|---------|
| 1x | M5Stack CoreS3 SE | https://www.digikey.de/de/products/detail/m5stack-technology-co-ltd/K128-SE/23628221?s=N4IgTCBcDaILIFYDKAXAhgYwNYAIDCA9gE4CmSAzDkgKIgC6AvkA | $47|
| 2x | M3x25mm Hex Head Cap Bolt | https://www.amazon.com/dp/B09NR8X2LV | $17.99 |
| 4x | M3x20mm Hex Head Cap Bolt | Comes as part of the set mentioned above | " | 
| 4x | Heat Set inserts M3 | https://www.amazon.com/ruthex-M3-Threaded-Inserts-RX-M3x5-7/dp/B08BCRZZS3 | $10.99 |
| 1x | 3.7v 2000mAh Lipo Battery Size 34.5 X 56 X 10.6 mm (The wires will need to be reversed in the connector on this one! See Assembly instructions for more info.) | https://www.amazon.com/EEMB-2000mAh-Battery-Rechargeable-Connector/dp/B08214DJLJ/ | $14.99 |
| 4x | Encoder Knob Bought or 3D Printed | https://www.aliexpress.us/item/3256801207971662.html?gatewayAdapt=deu2usa4itemAdapt | $5.00 |
| 1x | M5 Remote PCB | KinyMaker Discord #M5-Remote Channel or https://www.pcbway.com/project/shareproject/M5Stack_Core2_Remote_Plattform_2cb5bac0.html | $30.00 |

## 3D Printed Parts Needed:

| Quantity | Part | Information |
|----------|------|-------------|
| 1x | M5_curved_w6mm+5.stl Thanks to "Hoodlatch" KM Discord | There is a specific version for the wider Adafruit LIPO battery. Print with the base side facing down, 6 walls 20% Infill | 
| 1x | TOP-*-Keycap-Standoff.stl | Top Depends on your Keycap: Cherry or DSA (DSA is wider) | 
| 4x | M5_Remote_Knob_Customizable.scad | If you go for the 3d Printed knobs |

Filament - A good quality PLA works well. While there are no threads it is recommended that your printer is well calibrated.  
