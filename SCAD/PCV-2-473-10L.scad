difference(){
  color("RosyBrown")cylinder(d=22,h=25,$fn=30);
  cylinder(d=3,h=27,$fn=15);
}
color("Black")difference(){
color("Black")cylinder(d=25.4,h=3,$fn=40);
  cylinder(d=3,h=27,$fn=15);
}
color("Black")difference(){
color("Black")translate([0,0,25.4-3])cylinder(d=25.4,h=3,$fn=40);
  cylinder(d=3,h=27,$fn=15);
}

color("Silver")translate([17.78/2,0,-3])cylinder(d=1.37,h=5,$fn=10);
color("Silver")translate([-17.78/2,0,-3])cylinder(d=1.37,h=5,$fn=10);