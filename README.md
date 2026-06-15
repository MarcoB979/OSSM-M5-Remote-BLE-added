# Project OSSM M5 REMOTE
## Overview of the OSSM-M5-Remote

A Remote Control Platform for with a focus on the [OSSM](https://github.com/KinkyMakers/OSSM-hardware) and other related ESP Controlled Sex Toys (like Eject cumpump and Fist-IT).

![Final Addon](image/remote.png?raw=true "Remote" )

Intially developed for the [OSSM Project](https://github.com/KinkyMakers/OSSM-hardware) by [Ortlof](https://github.com/ortlof), this new firmware has received a complete re-do.


To help with development and design join the [KinkyMakers Discord](https://discord.gg/MmpT9xE). Be sure to say hello in the #m5-remote channel. 

## M5 Remote now works with the official OSSM BLE firmware. ##
If you still have the 'old' ESP_NOW firmware on your OSSM (used for previous M5 remote firmwares), from the [esp-now branch](https://github.com/ortlof/OSSM-Stroke), this new M5 remote firmware will still work. Communication via ESP_NOW still is possible, but some extra functionality will be limited (no streaming mode for example). If you use the newer V2 OSSM controller boards, please update your OSSM Firmware.

The Cable remote is obsolete with this firmware.

## Supported in this version of the M5 remote:
[OSSM latest firmware (BLE)](https://github.com/KinkyMakers/OSSM-hardware)  
[OSSM StrokeEngine branch for older (esp_now) OSSM](https://github.com/KinkyMakers/OSSM-hardware)

## Additional toy support:

[EJECT Cumpump](https://github.com/MarcoB979/EJECT-Cumpump)  [A Work in Progress]


[FIST-IT](https://github.com/MarcoB979/Fist-IT) | [A Work in Progress]

For information on how to build and source your materials, see the bottom of the page or see the official M5 remote page here:
[](https://github.com/ortlof/OSSM-M5-Remote)

## Operation, functionality and screens:
Steps to initialize start-up:
1. Power on the OSSM and let it home.
2. Power on the OSSM M5 Remote.
3. M5 remote will try an auto connect first. If the OSSM was not ready and connection is unsuccesfull you can try again manually by pressing the left encoder to select 'Connect'.
4. After successfull connection, the menu screen will show.


On the top left of the screen, you can see the status icons. If the OSSM is connected via Bluetooth you will see a Bluetooth icon, if it is connected via ESP_NOW you will see a WiFi icon. If addons are connected, you will also see these here. When the OSSM is performing a homeing sequence, you will see a homeing icon (only in BLE mode)


## Menu screen
<img src="image/Menu.jpg?raw=true" alt="Final Addon" title="Menu" width="200">
In the menu screen you can select the various options and go to the respective screens:
- Home (OSSM Control)
- Bator Mode (OSSM control for strokers)
- Settings (to change the different settings)
- Addons (to open and/or activate (show/hide) the available addons.
    - Available addons at this moment: Eject, Fist-IT and Streaming mode


* make your selection by rotating the right encoder.


* Pressing the left encoder button makes the M5 remote restart (after you confirmed the notification)
* Pressing the middle (MX/square) button, you can change the colors used in the screen (ui themes are built in)
* Pressing the right encoder button selects the actively selected menu option

## (OSSM) Home screen
<img src="image/OSSM-home.jpg?raw=true" alt="home" title="Home" width="200">
This is the home screen where you can control your OSSM after successfull connection. The OSSM is controlled in 'Stroke Engine' mode.


By rotating the left encoder you control speed
By rotating the second encoder you change depth
By rotating the third encoder you change stroke (notice the slider shows the stroke depth relative to the depth)
By rotating the fourth (right) encoder you can change sensation (influences pattern behaviour)


Rotating an encoder to the right increases (+) the values, to the left the values decreases (-) the values. If in settings you selected to invert the stroke, rotating the slider increases when rotating to the right, if unchecked the values will decrease.


If stroke, speed and depth are changed to all positive values the OSSM will start automatically. if one of the values is set to 0, the OSSM will stop.


You can also press the middle button to start and stop.


When long pressing the middle button, you activate the emergency stop: the OSSM will slowly retract itself and go back to the menu screen. The OSSM MUST be re-homed before you can start to play again.


When pressing the left button, the OSSM will only do the pullout movement as described, but not trigger an emergency stop or go back to the menu.


To change a pattern, click the right button to go to the Pattern selection screen.


## Bator Mode
<img src="image/Bator-mode.jpg?raw=true" alt="bator" title="Bator mode" width="200">
Bator mode has the same functionality as the OSSM home screen. However, the OSSM movement is now suitable for strokers. The OSSM will now move with the middle of the rail as starting point. 


Only speed, stroke and sensation can be set. When stroke is changed, the OSSM will move from middle to the size of stroke. If stroke is set to 20% as example, the OSSM will move from middle of the rail (50%) to 10% to the left (40%) and then to 60% of the rail lengte (making it a 20% stroke move)


Clicking the right button brings you in the pattern selection menu. Clicking the left encoder you will return to the menu screen.
The middle square button has the same start/stop behaviour as the OSSM home screen, including the long press emergency stop.


## Settings
<img src="image/Settings.jpg?raw=true" alt="settings" title="Settings" width="200">
In settings you can change the following settings:
-Vibrate: should the M5 Remote vibrate after button presses to give haptic feedback. 
-Touch Disabled: if you do not want to be able to use the touchscreen, then check this option.
-Stroke inverted: change the bravoure of the encoder where you change the stroke values in the OSSM Ho.e screen (rotating right games stoke go up, or down.
-Force re-home: if we switch to the Menu screen and then back to screens where you can control the OSSM, this can force a homeing procedure. This is a safety measure, but sometimes this might be not totally necessary if you use the 57AIM Gold Motor. In that case you can choose to disable to force re-homes on occasions where this is not absolutely necessary. 

> [!WARNING]
PLEASE NOTE: it is recommended to have this selected. EXTRA CAUTION: if you own the IHSV57 motor, you MUST keep this option activated for safety reasons!


You can also change the screen brightness to fit your needs and save on battery life. To do this, rotate the 3rd rotary encoder (seen from the left). Changes are applied immediately.

As a standard, the M5 remote has a screensaver functionality.  if not using the remote, the screen will be dimmed, after a longer time of not using, the remote will go into deep sleep. This will disconnect all connections too.


By pressing the left encoder, you save the settings in memory. The M5 Remote will use these saved settings at next start-up.
Rotating the right encoder will scroll through the options. Pressing it will select or de-select the active option


By pressing the middle button, you return to the menu screen. Do not forget to save first.


## Addons screen
<img src="image/Addons.jpg?raw=true" alt="Addons" title="Addons" width="200">
The new M5 Remote has several addons available, which you can start in the Addons screen. 


To select (open) an addon, navigate to it and select/run it by pressing the right encoder button. 


If you dont want to use an addon, you can hide (deactivate) it (or show/activate if you want to undo this). 
After you press the middle button (show/hide) you can deactivate (hide) or activate (show) the addon in the selection menu. You can also show/activate all available addons at once by pressing the show all button.


Your choice will be stored in memory so you only have to do this once.


# Addons - Streaming mode
<img src="image/Streaming.jpg?raw=true" alt="Streaming" title="Streaming" width="200">

The latest OSSM Firmware has a streaming mode. For example: by using streaming mode, you are able to use funscripts or the xtoys application (position mode).


Xtoys is however not completely compatible/functional yet (there is no starting speed values sent by xtoys after position/streaming mode is made active). By using the M5 Remote, this is performer correctly, or you can manage this yourself. 


Ater you start the M5 Streaming mode, the M5 will send the task to go streaming to the OSSM, make sure it will do a homeing procedure when necessary and then slowly push out the rail to the maximum. this way you can position yourself safely.
This is done in a few steps. Notifications on screen will help/guide you.


Some of these notifications show safety warnings, since the OSSM Streaming mode still is an experimental feature. So is the addon available in the M5 Remote. **PLEASE BE CAUTIOUS**  when using streaming mode. Ensure patterns, (fun)scripts and others are tested and found safe by you before use. **You alone are responsible for your own safety!**


Normally, when the streaming setup is finished, you can safely shut down the M5 remote. There is a possibility to override settings. This way you can change the maximum speed, depth or stroke the OSSM will accept and you can start or stop movement. Changing max speed (or other values) will not make changes to the settings of the online streaming service. So if in the streaming service (xtoys as example) the speed is set at 80 and on your M5 max speed is set at 50 (%) the OSSM will set the actual speed at 40 (50% of the set 80). This works similar to stroke, depth and sensation.

> [!WARNING]
> Overriding can give you more control, but also can pose a safety hazard if done pourly. **ONLY** use this functionality if you understand the hazards and know what you do. Again: ONLY YOU are responsible for your safety.


# Addons - EJECT cumpump
<img src="image/Eject.jpg?raw=true" alt="Eject" title="Eject" width="200">
For code and information, see my  [Eject Cumpump repository](https://github.com/MarcoB979/Eject)


The EJECT Cumpump  is based on the eject cumpump from [Ortlof](https://github.com/ortlof/EJECT-cum-tube-project). I have made a functional firmware which is integrated in the M5 remote. It can be activated in the Addons screen by showing (or hiding) the addon.


The screen itself lets you configure the squirt pattern which executes once. Set your desired speed, how many times a shot (squirt) should happen, the volume of each shot and the force (accelleration: how fast the speed ramps up). If the force is the same than the speed, the pump will turn instantantly, If it is lower, the speed will ramp up. If you set this too low, the Eject possibly will not reach the set speed. 


If all values have been correctly set, you can start the squirt sequence by pressing the middle 'cum' button.
> [!TIP] If you have enabled the Eject Cumpump in the Addons screen and the Eject Cumpump is turned on and connected to the M5 remote, you will see the status icon on the top left. Also the left button in the home screen will have an added 'E' to the button text. When long pressing the left button in home screen, you will open the Eject screen. If you double click (leave 0.5 seconds between clicks) in the home screen, you will start the squirt sequence you have previously configured (similar to pressing the 'CUM' button in the eject screen)
>

# Addons - Fist-IT   <img src="image/Fist-IT.jpg?raw=true" alt="Fist-IT" title="Fist-IT" width="200">
For code and information, see my  [Fist-IT repository !](https://github.com/MarcoB979/EJECT-Cumpump)

Fist-IT is an attachment I designed, which you can mount on your OSSM using the 24mm thread. The Fist-IT is an enclosed, geared, Nema-23 motrlor and driver, run by an ESP32 Super mini. You can then attach a fisting dildo, to mimic the rotational movement when one is being fisted.

The screen lets you set all the necessary parameters by rotating the encoders:
- Speed
- Rotation (0-360)
- Pause in 0.1 seconds per steps. This adds a pause between each movemet, both backwards or forwards
- Accell which sets the accelleration of the movement.

> [!IMPORTANT]
Be mindfull of the settings and do 'dry testruns' before you use the Fist-IT since if using in a wrong way, this can cause **serious injuries**
>

If all values have been correctly set, you can start the rotational movemt by pressing the middle 'Start/stop' button. Pressing it again will stop the movement instantly. A new press will again start the movement.
> [!TIP] if you have enabled the Fist-IT in the Addons screen and the Fist-IT is turned on and connected to the M5 remote you will see the status icon on the top left. Also the right button in the home screen will have an added 'F' to the button text. When long pressing the left button in home screen, you will open the Fist-IT screen. If you double click (leave 0.5 seconds between clicks) in the home screen, you will start the Fist-IT (or stop after a second double click) using the values you have previously configurered in the Fist-IT screen (similar to pressing the 'Start/Stop' button in the Fist-IT screen)
> 




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
