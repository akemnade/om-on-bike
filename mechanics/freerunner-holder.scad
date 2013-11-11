/* freerunner bicycle holder. screw it on the lower half of a bell */

freerunner_width=62+1.5;
freerunner_height=120.7;
freerunner_thickness=18.5+1+1;

m4_hole_r=4.3/2;
m4_nut_r=7.8/2;
module freerunner_eckig() {
  cube([freerunner_width,freerunner_height,freerunner_thickness]);
//  translate(v=[10,10,5])
//  minkowski() 
//  {
//   cube([freerunner_width-20,freerunner_height-20,freerunner_thickness-10]);
//    scale([1,1,0.5]) sphere(r=10);
//  }
}

module freerunner_rund() {
  union() {
  /* hole for usb */
  translate(v=[0,34,3]) cube([freerunner_width+20,21,11]);  
  translate(v=[0,freerunner_width/2,0])
  cube([freerunner_width,freerunner_height-freerunner_width/2,freerunner_thickness]);
  translate(v=[freerunner_width/2,freerunner_width/2,0])
  cylinder(r=freerunner_width/2,h=freerunner_thickness);
}
}

module freerunner_holder() {

difference() {
  //cube([freerunner_width+8,freerunner_height-20+8,freerunner_thickness+10]);
  union() {

  translate(v=[0,(freerunner_width+8)/2,0])
    cube([freerunner_width+8,freerunner_height-20+8-(freerunner_width+8)/2,freerunner_thickness+11]);
  translate(v=[(freerunner_width+8)/2,(freerunner_width+8)/2,0])
    cylinder(r=(freerunner_width+8)/2,h=freerunner_thickness+11);
  
}
  
  translate(v=[4,4,7])
  freerunner_rund();
  //cube(size=[freerunner_width,freerunner_height,freerunner_thickness]);
  /* lower slot */
  translate(v=[0.5,0,2.5])
  cube(size=[3,freerunner_height,2.5]);
 /* lower slot */
 translate(v=[freerunner_width+8-3-0.5,0,2.5])
  cube(size=[3,freerunner_height,2.5]);
 /* unneeded material */
  translate(v=[16,20,1.5]) 
   cube(size=[freerunner_width-26,freerunner_height/2-25,10]);
 /* unneeded material */
 translate(v=[16,13+freerunner_height/2,1.5]) 
   cube(size=[freerunner_width-26,20,10]);
 /* display opening, leave a thin layer for easier printing */
  translate(v=[4+5,4+15,7+freerunner_thickness+0.5]) cube(size=[freerunner_width-10,freerunner_height-15,freerunner_thickness+10]);
 /* upper slot */ 
 translate(v=[0.5,0,freerunner_thickness+7+1.5])
  cube(size=[1.5,freerunner_height,1.5]);
 /* upper slot */
translate(v=[freerunner_width+8-0.5-1.5,0,freerunner_thickness+7+1.5])
  cube(size=[1.5,freerunner_height,1.5]);

  /* screwholes */
  translate(v=[freerunner_width/2+3,freerunner_height/2+4,0]) {
     translate(v=[-25/2,0,0]) {
        cylinder(r=m4_hole_r,h=20);
        translate(v=[0,0,3]) cylinder(r=m4_nut_r,$fn=6,h=5);
     }
      translate(v=[25/2,0,0]) {
        cylinder(r=m4_hole_r,h=20);
         translate(v=[0,0,3]) cylinder(r=m4_nut_r,$fn=6,h=5);
       
        //translate(v=[-1,-3.5,-8])  cube([3,7,20]); 
     }
  } 
}
}


/*projection() intersection () {
 translate(v=[-20,-20,10]) cube([200,200,0.5]);
freerunner_holder();
}
*/


freerunner_holder();