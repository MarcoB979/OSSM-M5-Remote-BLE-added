> [!NOTE]
>- 	[Quick start guide](/src/Manuals-templates/M5%20Remote%20Quick%20Start%20guide.md)
>
>- 	[Manual extra explained](/src/Manuals-templates/M5%20Remote%20Manual%20-%20extra%20explanation.md)
>
>- [Web flasher page](https://marcob979.github.io/OSSM-M5-Remote-BLE-added/webflasher/)



The M5 remote is a Remote Control Platform for with a focus on the [OSSM](https://github.com/KinkyMakers/OSSM-hardware) and other related ESP Controlled Sex Toys (like Eject cumpump and Fist-IT).

![Final Addon](image/remote.png?raw=true "Remote" )

Intially developed for the [OSSM Project](https://github.com/KinkyMakers/OSSM-hardware) by [Ortlof](https://github.com/ortlof), this new firmware has received a complete makeover and adds bluetooth functionality. The new M5 remote firmware will work with the latest stock (BLE) OSSM firmware and OSSM-Lite. Rust-OSSM has not been tested yet.



## Before You Start

1. Charge your M5 Remote.
2. Power on your OSSM.
3. Keep your hand close to the M5 buttons during first tests.

> [!TIP]
> Start every new setup with low speed, low depth, and low stroke.

## Quick Online Flash (Web Flasher)

You can flash firmware directly from browser.

Open:
- [Web flasher page](https://marcob979.github.io/OSSM-M5-Remote-BLE-added/webflasher/)

Steps:
1. Connect your M5 with a USB data cable.
2. Select your board (Core2 or CoreS3).
3. Click Install.
4. Select the COM/serial port.
5. Wait for flash to finish, then reboot the device.

> [!TIP]
> Use Chrome or Edge on desktop for best compatibility.

> [!WARNING]
> Always select the correct board type before flashing.

---

## User manuals:
- 	[Quick start guide](src/Manuals-templates/M5%20Remote%20Quick%20Start%20guide.md)

- 	[Manual extra explained](src/Manuals-templates/M5%20Remote%20Manual%20-%20extra%20explanation.md)


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