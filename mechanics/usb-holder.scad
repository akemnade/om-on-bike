diam = 22;
cylh = 8;
gap = 8;
thickness =3;
thickness_strong = 6;
usb_l=15+2;
usb_h=17.75;
usb_h_outer=21;
usb_h_diff=(usb_h_outer-usb_h)/2;
usb_b=16.25;
usb_b_outer=30.5;
cable_gap=4;
module usb_socket()
{
   union() {
     difference() {
       cube([usb_l+4,usb_h_outer+cable_gap+8,usb_b_outer+8]);
       translate(v=[0,4+usb_h_diff,(usb_b_outer-usb_b)/2+4]) cube([usb_l+4,usb_h,usb_b]);
       translate(v=[0,4,4]) cube([usb_l,usb_h_outer+cable_gap,usb_b_outer]);
       translate(v=[usb_l-1,4+9+6,4+usb_b_outer-(30.5-24)/2]) rotate([0,90,0]) cylinder(r=1.6,h=4+2);
       translate(v=[usb_l-1,4+6,4+(30.5-24)/2]) rotate([0,90,0]) cylinder(r=1.6,h=4+2);
 
    }
    /* translate(v=[usb_l-6,usb_h+4,0])
    cube([6,cable_gap+3,usb_b+8]); */
    difference() {
      translate(v=[-7.9,usb_h_outer+cable_gap+4,0]) cube([8,7,usb_b_outer+8]);
      translate(v=[-7.9,usb_h_outer+cable_gap+4,usb_b_outer/2+4]) rotate([0,90,0]) cylinder(r=2.3,h=10);
      translate(v=[-7.9+4,usb_h_outer+cable_gap+4-0.1,usb_b_outer/2+4+4+1.6]) rotate([-90,0,0]) cylinder(r=1.6,h=20);
      translate(v=[-7.9+4,usb_h_outer+cable_gap+4-0.1,usb_b_outer/2+4-4-1.6]) rotate([-90,0,0]) cylinder(r=1.6,h=20);
    }
   }
}

module usb_holder() {
union() {
difference() {
union() {
cylinder(r=(diam/2)+thickness, h=cylh);
translate(v=[-diam/2-10-thickness,-gap/2-thickness_strong,0])
cube([30,gap+2*thickness_strong,cylh]);
translate(v=[-cylh/2,0,0])
cube([cylh,diam/2+7,cylh]);
}
cylinder(r=diam/2, h=cylh);
translate(v=[-diam/2-10-thickness,-gap/2,0])
cube([30,gap,cylh]);
translate(v=[-diam/2-5,-gap/2-thickness_strong,cylh/2])
rotate([-90,0,0]) cylinder(r=2.2, h=gap+thickness_strong*2, $fn=15);
}
translate([-cylh/2,diam/2+5,0])
 usb_socket();
}
}

module klemme()
{
  difference() {
  union() {
    cube([usb_b_outer+8,usb_h_outer+3,1.4]);
    translate(v=[usb_b_outer/2+4-4-6,0]) cube([4+6+4+6,4,8]);
    translate(v=[usb_b_outer/2+4,0,0]) cylinder(r=4,h=8);
  } 
  translate(v=[usb_b_outer/2+4,-1,0]) cylinder(r=2.3,h=8);
  translate(v=[usb_b_outer/2+4+4+1.6,0,4]) rotate([-90,0,0]) cylinder(r=1.6,h=20);
  translate(v=[usb_b_outer/2+4-4-1.6,0,4]) rotate([-90,0,0]) cylinder(r=1.6,h=20);
  translate(v=[0,-10,0]) cube([usb_b_outer+8,10,20]);
  }
}

/* print with something hard */
klemme();

/* used soft pla for that */
translate(v=[30,50,0])
usb_holder();
/*
 projection() rotate([0,90,0]) intersection() {
 translate(v=[18,0,0]) cube([0.5,50,50]);
usb_holder();

}
 projection() {translate(v=[44,0,0])  rotate([0,90,0]) intersection() {
 translate(v=[14.2,0,0]) cube([0.5,50,50]);
usb_holder();

}
}
*/
