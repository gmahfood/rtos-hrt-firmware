//
// rtos-hrt-firmware enclosure - updated Rev A
// Parametric OpenSCAD starter for 87x134 mm PCB
// Rounded lid corners + chamfered front cutouts
//

$fn = 64;
eps = 0.01;

// =========================
// PARAMETERS
// =========================

// PCB
pcb_w = 87;
pcb_l = 134;
pcb_th = 1.6;

// Fit clearances
pcb_side_clear = 4.0;
pcb_top_clear  = 18.0;

// Shell
wall      = 3.0;
floor_th  = 3.0;
lid_th    = 3.0;
corner_r  = 6.0;

// Base/lid proportions
front_h   = 22;
rear_h    = 52;
base_h    = 16;

// Derived
inner_w = pcb_w + pcb_side_clear * 2;
inner_l = pcb_l + pcb_side_clear * 2;

outer_w = inner_w + wall * 2;
outer_l = inner_l + wall * 2;

// PCB mounting
standoff_d = 8;
standoff_h = 6;
standoff_hole_d = 2.7;
pcb_mount_inset_x = 5;
pcb_mount_inset_y = 5;

// Lid screw posts
post_d = 10;
post_h = base_h - 1;
post_hole_d = 3.0;
post_offset = 10;

// Front panel controls
lcd_view_w = 71;
lcd_view_h = 24;

rgb_slot_w = 72;
rgb_slot_h = 5;

button_d   = 12.2;
button_gap = 18;
button_count = 3;

pot_hole_d = 7.5;
seg_win_w  = 30;
seg_win_h  = 14;

// Layout on front face
panel_y_from_front = 34;
lcd_y_from_front   = 20;
rgb_y_from_front   = 29;

// X positions on control row
pot_x = 34;
seg_x = 22;

// Rear cutouts
usb_cut_w = 14.5;
usb_cut_h = 16.0;
usb_cut_z = 8;

// Print/debug
show_base = true;
show_lid = true;
exploded = true;
explode_z = 30;

show_test_panel = false;
test_panel_depth = 45;

// =========================
// HELPERS
// =========================

module rounded_rect_2d(w, l, r=4) {
    hull() {
        translate([ w/2-r,  l/2-r]) circle(r=r);
        translate([-w/2+r,  l/2-r]) circle(r=r);
        translate([-w/2+r, -l/2+r]) circle(r=r);
        translate([ w/2-r, -l/2+r]) circle(r=r);
    }
}

module rounded_box(w, l, h, r=4) {
    linear_extrude(height=h)
        rounded_rect_2d(w, l, r);
}

module screw_posts() {
    for (x = [-outer_w/2 + post_offset, outer_w/2 - post_offset])
    for (y = [-outer_l/2 + post_offset, outer_l/2 - post_offset]) {
        translate([x, y, floor_th])
        difference() {
            cylinder(h=post_h, d=post_d);
            translate([0,0,-0.1]) cylinder(h=post_h + 0.2, d=post_hole_d);
        }
    }
}

module pcb_standoffs() {
    for (x = [-pcb_w/2 + pcb_mount_inset_x, pcb_w/2 - pcb_mount_inset_x])
    for (y = [-pcb_l/2 + pcb_mount_inset_y, pcb_l/2 - pcb_mount_inset_y]) {
        translate([x, y, floor_th])
        difference() {
            cylinder(h=standoff_h, d=standoff_d);
            translate([0,0,-0.1]) cylinder(h=standoff_h + 0.2, d=standoff_hole_d);
        }
    }
}

// Approximate panel plane angle
panel_angle = atan((rear_h - front_h) / outer_l);

// Put geometry onto the lid slope
module panel_cut(y_from_front, z_offset=0) {
    translate([0, -outer_l/2 + y_from_front, front_h + z_offset])
        rotate([90 - panel_angle, 0, 0])
            children();
}

// Chamfered rectangular cut
module chamfered_rect_cut(w, h, cut_depth=10, chamfer=1.0, chamfer_depth=1.0) {
    union() {
        linear_extrude(height=cut_depth)
            square([w, h], center=true);

        linear_extrude(
            height=chamfer_depth,
            scale=[(w + 2*chamfer)/w, (h + 2*chamfer)/h]
        )
            square([w, h], center=true);
    }
}

// Chamfered circular cut
module chamfered_circle_cut(d, cut_depth=10, chamfer=1.0, chamfer_depth=1.0) {
    union() {
        cylinder(h=cut_depth, d=d);
        cylinder(h=chamfer_depth, d1=d + 2*chamfer, d2=d);
    }
}

// =========================
// BASE
// =========================

module base_shell() {
    difference() {
        rounded_box(outer_w, outer_l, floor_th + base_h, corner_r);

        translate([0,0,floor_th])
            rounded_box(inner_w, inner_l, base_h + 0.1, corner_r-2);

        // Rear USB cutout
        translate([0, outer_l/2 + 0.05, floor_th + usb_cut_z])
            rotate([90,0,0])
                linear_extrude(height=wall + 0.2)
                    square([usb_cut_w, usb_cut_h], center=true);
    }

    pcb_standoffs();
    screw_posts();
}

// =========================
// LID
// =========================

module lid_outer() {
    hull() {
        // front slice
        translate([0, -outer_l/2 + eps, 0])
            linear_extrude(height=front_h)
                rounded_rect_2d(outer_w, eps*2, corner_r);

        // rear slice
        translate([0, outer_l/2 - eps, 0])
            linear_extrude(height=rear_h)
                rounded_rect_2d(outer_w, eps*2, corner_r);
    }
}

module lid_inner() {
    inner_lid_w = outer_w - 2*wall;
    inner_lid_l = outer_l - 2*wall;
    inner_front_h = front_h - lid_th;
    inner_rear_h  = rear_h - lid_th;
    inner_r = max(corner_r - wall, 0.5);

    hull() {
        // front inner slice
        translate([0, -inner_lid_l/2 + eps, lid_th])
            linear_extrude(height=inner_front_h)
                rounded_rect_2d(inner_lid_w, eps*2, inner_r);

        // rear inner slice
        translate([0, inner_lid_l/2 - eps, lid_th])
            linear_extrude(height=inner_rear_h)
                rounded_rect_2d(inner_lid_w, eps*2, inner_r);
    }
}

module lid_shell() {
    difference() {
        lid_outer();
        lid_inner();

        // LCD window
        panel_cut(lcd_y_from_front, 0)
            chamfered_rect_cut(
                lcd_view_w, lcd_view_h,
                cut_depth=10,
                chamfer=1.2,
                chamfer_depth=1.2
            );

        // RGB slot
        panel_cut(rgb_y_from_front, -1)
            chamfered_rect_cut(
                rgb_slot_w, rgb_slot_h,
                cut_depth=10,
                chamfer=0.8,
                chamfer_depth=0.8
            );

        // Buttons
        for (i = [0:button_count-1]) {
            x = (i - (button_count-1)/2) * button_gap;
            translate([x,0,0])
                panel_cut(panel_y_from_front, -1)
                    chamfered_circle_cut(
                        button_d,
                        cut_depth=10,
                        chamfer=0.8,
                        chamfer_depth=0.8
                    );
        }

        // Pot hole
        translate([pot_x,0,0])
            panel_cut(panel_y_from_front, -1)
                chamfered_circle_cut(
                    pot_hole_d,
                    cut_depth=10,
                    chamfer=0.8,
                    chamfer_depth=0.8
                );

        // 7-segment window
        translate([seg_x,0,0])
            panel_cut(panel_y_from_front, -1)
                chamfered_rect_cut(
                    seg_win_w, seg_win_h,
                    cut_depth=10,
                    chamfer=1.0,
                    chamfer_depth=1.0
                );

        // Screw holes
        for (x = [-outer_w/2 + post_offset, outer_w/2 - post_offset])
        for (y = [-outer_l/2 + post_offset, outer_l/2 - post_offset]) {
            translate([x, y, -0.1])
                cylinder(h=rear_h + 2, d=post_hole_d + 0.4);
        }
    }
}

// Narrow test piece from the front area only
module front_panel_test_piece() {
    intersection() {
        lid_shell();
        translate([0, -outer_l/2 + test_panel_depth/2, rear_h/2])
            cube([outer_w + 2, test_panel_depth, rear_h + 2], center=true);
    }
}

// =========================
// RENDER
// =========================

if (show_test_panel) {
    front_panel_test_piece();
} else {
    if (show_base)
        base_shell();

    if (show_lid) {
        if (exploded) {
            translate([0,0,floor_th + base_h + explode_z])
                lid_shell();
        } else {
            translate([0,0,floor_th + base_h])
                lid_shell();
        }
    }
}