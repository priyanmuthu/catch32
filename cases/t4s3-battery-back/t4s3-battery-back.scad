/*
  LILYGO T4-S3 battery back case

  This is a parametric first-pass back shell for the T4-S3 with the front
  case/female-pin option. Measure your board, front shell, and battery before
  printing the final version.

  Units: millimeters
*/

$fn = 48;

// --------------------
// User-tunable dimensions
// --------------------

// Outer footprint of the rear case. Start slightly larger than the front case.
case_w = 72.0;
case_h = 58.0;
case_corner_r = 5.0;

// Rear shell geometry.
wall = 1.8;
floor_th = 1.8;
case_depth = 10.5;
front_lip_h = 2.2;
front_lip_inset = 1.0;

// Battery pocket. Default is based on a small flat 3.7 V LiPo class cell.
// Replace these with measured dimensions from your exact Amazon battery.
battery_w = 26.5;
battery_h = 20.5;
battery_t = 4.3;
battery_clearance = 0.8;
battery_pocket_wall = 1.2;

// Battery pocket position, centered horizontally and biased toward the bottom.
battery_center_x = 0;
battery_center_y = -13;

// Approximate mounting bosses. These are intentionally configurable because
// the front-case/female-pin variant should be verified against the real part.
boss_d = 5.4;
boss_h = 5.0;
screw_hole_d = 2.1;     // M2 self-tapping pilot / M2 clearance depending print.
boss_inset_x = 7.0;
boss_inset_y = 7.0;

// Ports and access cutouts. Adjust after a test fit.
usb_cutout_w = 14.0;
usb_cutout_h = 7.0;
sd_cutout_w = 18.0;
sd_cutout_h = 4.0;
qwiic_cutout_w = 17.0;
qwiic_cutout_h = 5.0;

// Cable channel from battery pocket toward the board battery connector.
cable_channel_w = 5.0;
cable_channel_len = 18.0;

// Vent slots.
vent_slot_w = 2.0;
vent_slot_h = 16.0;
vent_spacing = 5.0;
vent_count = 5;

// Render toggles.
show_board_placeholder = false;
show_battery_placeholder = true;

// --------------------
// Helpers
// --------------------

module rounded_rect_2d(w, h, r) {
  hull() {
    translate([w / 2 - r, h / 2 - r]) circle(r = r);
    translate([-w / 2 + r, h / 2 - r]) circle(r = r);
    translate([w / 2 - r, -h / 2 + r]) circle(r = r);
    translate([-w / 2 + r, -h / 2 + r]) circle(r = r);
  }
}

module rounded_box(w, h, d, r) {
  linear_extrude(height = d) rounded_rect_2d(w, h, r);
}

module case_shell() {
  difference() {
    rounded_box(case_w, case_h, case_depth, case_corner_r);

    translate([0, 0, floor_th])
      rounded_box(
        case_w - 2 * wall,
        case_h - 2 * wall,
        case_depth + 0.2,
        max(0.1, case_corner_r - wall)
      );

    // Shallow front mating lip relief.
    translate([0, 0, case_depth - front_lip_h])
      rounded_box(
        case_w - 2 * front_lip_inset,
        case_h - 2 * front_lip_inset,
        front_lip_h + 0.3,
        max(0.1, case_corner_r - front_lip_inset)
      );
  }
}

module standoff(x, y) {
  translate([x, y, floor_th]) {
    difference() {
      cylinder(d = boss_d, h = boss_h);
      translate([0, 0, -0.1]) cylinder(d = screw_hole_d, h = boss_h + 0.2);
    }
  }
}

module standoffs() {
  standoff(case_w / 2 - boss_inset_x, case_h / 2 - boss_inset_y);
  standoff(-case_w / 2 + boss_inset_x, case_h / 2 - boss_inset_y);
  standoff(case_w / 2 - boss_inset_x, -case_h / 2 + boss_inset_y);
  standoff(-case_w / 2 + boss_inset_x, -case_h / 2 + boss_inset_y);
}

module battery_pocket() {
  pocket_w = battery_w + 2 * battery_clearance + 2 * battery_pocket_wall;
  pocket_h = battery_h + 2 * battery_clearance + 2 * battery_pocket_wall;
  pocket_inner_w = battery_w + 2 * battery_clearance;
  pocket_inner_h = battery_h + 2 * battery_clearance;
  pocket_h_total = battery_t + 1.4;

  translate([battery_center_x, battery_center_y, floor_th]) {
    difference() {
      rounded_box(pocket_w, pocket_h, pocket_h_total, 2.2);
      translate([0, 0, 1.2])
        rounded_box(pocket_inner_w, pocket_inner_h, pocket_h_total + 0.2, 1.6);

      // Finger notch to lift/remove the pouch.
      translate([0, -pocket_h / 2, 1.2])
        cylinder(d = 11, h = pocket_h_total + 0.4);
    }
  }
}

module cable_channel_cut() {
  translate([
    battery_center_x,
    battery_center_y + (battery_h / 2) + cable_channel_len / 2,
    floor_th + 0.6
  ])
    cube([cable_channel_w, cable_channel_len, 2.0], center = true);
}

module side_cutouts() {
  // USB-C cutout, centered on top edge by default.
  translate([0, case_h / 2, floor_th + 3.0])
    cube([usb_cutout_w, wall + 0.8, usb_cutout_h], center = true);

  // microSD access guess on left edge.
  translate([-case_w / 2, -3, floor_th + 4.2])
    cube([wall + 0.8, sd_cutout_w, sd_cutout_h], center = true);

  // Qwiic access guess on right edge.
  translate([case_w / 2, 8, floor_th + 4.0])
    cube([wall + 0.8, qwiic_cutout_w, qwiic_cutout_h], center = true);
}

module vent_cuts() {
  for (i = [0 : vent_count - 1]) {
    x = (i - (vent_count - 1) / 2) * vent_spacing;
    translate([x, 13, -0.1])
      rounded_box(vent_slot_w, vent_slot_h, floor_th + 0.4, 0.8);
  }
}

module back_case() {
  difference() {
    union() {
      case_shell();
      standoffs();
      battery_pocket();
    }

    cable_channel_cut();
    side_cutouts();
    vent_cuts();
  }
}

module placeholders() {
  if (show_board_placeholder) {
    color([0.0, 0.35, 0.1, 0.35])
      translate([0, 0, floor_th + boss_h + 0.6])
        cube([case_w - 7, case_h - 7, 1.2], center = true);
  }

  if (show_battery_placeholder) {
    color([0.15, 0.15, 0.15, 0.45])
      translate([battery_center_x, battery_center_y, floor_th + 1.2 + battery_t / 2])
        cube([battery_w, battery_h, battery_t], center = true);
  }
}

back_case();
placeholders();
