# T4-S3 Battery Back Case

This folder contains a parametric OpenSCAD starter design for a 3D-printable rear case for the LILYGO T4-S3 with the front case/female-pin option.

Device reference: [../../devices/T4S3/README.md](../../devices/T4S3/README.md)

Case model: [t4s3-battery-back.scad](t4s3-battery-back.scad)

Rendered STL: [t4s3-battery-back.stl](t4s3-battery-back.stl)

## Current Assumptions

The Amazon battery page did not expose reliable dimensions through the tools available here, so the model uses editable defaults for a small flat 3.7 V LiPo-style pouch:

```scad
battery_w = 26.5;
battery_h = 20.5;
battery_t = 4.3;
battery_clearance = 0.8;
```

Before printing the final case, measure your actual battery with calipers and update those values.

Also measure the assembled T4-S3 front case and board stack, then tune:

```scad
case_w = 72.0;
case_h = 58.0;
case_depth = 10.5;
boss_inset_x = 7.0;
boss_inset_y = 7.0;
```

Treat the first print as a mechanical fit test.

## What The Model Includes

- Rounded rectangular rear shell.
- Hollow interior.
- Shallow front mating lip relief.
- Four configurable standoff/screw bosses.
- Battery pocket with clearance.
- Battery cable channel.
- USB-C, microSD, and Qwiic side cutout placeholders.
- Vent slots.
- Optional visual placeholders for the battery and board.

The side cutouts and screw boss positions are guesses until checked against the actual front shell and board.

## Recommended Workflow

1. Download LILYGO's official mechanical files:
   - https://github.com/Xinyuan-LilyGO/LilyGo-AMOLED-Series/tree/master/dimensions
   - https://github.com/Xinyuan-LilyGO/LilyGo-AMOLED-Series/tree/master/shell

2. Open `t4s3-battery-back.scad` in OpenSCAD.

3. Measure and update:
   - Battery width, height, and thickness.
   - Front case outer width and height.
   - Required back depth.
   - Mounting hole or screw boss positions.
   - USB-C, microSD, Qwiic, BOOT/RST, and female-header access positions.

4. Render in OpenSCAD with `F6`.

5. Export STL:

   ```sh
   /opt/homebrew/bin/openscad -o t4s3-battery-back.stl t4s3-battery-back.scad
   ```

6. Print a low-quality draft first:
   - Layer height: `0.24 mm`
   - Walls: `3`
   - Infill: `15-20%`
   - Material: PETG preferred, PLA acceptable for fit checks

7. Test fit with no battery connected.

8. Revise the model, then print the final version.

## Battery Safety

- Use only a single-cell `3.7 V` Li-ion/LiPo battery.
- Confirm connector polarity before plugging into the T4-S3.
- Do not compress, bend, puncture, or tightly clamp a pouch cell.
- Leave at least `0.5-1.0 mm` clearance around the battery.
- Use foam tape or a light printed strap for retention.
- Keep the case removable so the battery can be inspected or replaced.

## Design Notes

The battery should sit behind the board in a shallow pocket, with a cable channel leading toward the T4-S3 battery connector. The pocket should hold the cell from sliding around but should not squeeze it.

For the front case with female pins, pay close attention to header clearance. Female headers can make the back shell need more depth or side relief than a bare board.

Good first-print strategy:

1. Print only the rear tray.
2. Check outer fit against the front shell.
3. Check USB-C and microSD access.
4. Check battery pocket fit with the battery disconnected.
5. Check cable bend radius.
6. Only then assemble with the battery connected.
