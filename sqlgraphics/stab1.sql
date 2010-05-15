/*

This code has been modified somewhat from the original to run in a 2010 era environment;
specifically, the CREATE PHANTOM MACHINE block has been eliminated since phantom machines
didn't really replace virtual machines until about 2016 and weren't fully supported by
SQL until ~2018.  Instead, we just run in the root environment and delete everything we 
care about upfront if it exists.

Also, optimizations for the SQPU (Structured Query Processing Unit, or Skup-you) have been
largely ignored, with translations hacked in where needed and other "optimization" left in
situ without comment, so that some of this code may appear as odd to a modern reader as it 
would have to a programmer from 2010.  

While this might offend purists, the fact is the harware available in those days was so 
pathetically underpowered and unreliable that the difference is hardly noticable.

*/

--
-- Make a clean start
--

drop type  if exists color  cascade;
drop type  if exists vector cascade;
drop table if exists face   cascade;
drop type  if exists spot   cascade;

--
-- Define our types, tables, and helper functions
--

-- COLOR:
create type color as (r real, g real, b real, a real);
create function the_color(real,real,real) returns color as 'select $1,$2,$3,$1-$1;' language sql;

-- VECTOR:
create type vector as (x real, y real, z real);
create function the_vector(real,real,real) returns vector as $$
    select $1,$2,$3;
    $$ language sql;
create function dot(vector,vector) returns real as $$
    select $1.x*$2.x + $1.y*$2.y + $1.z*$2.z;
    $$ language sql;

-- FACE: A colored polygon at a given distance & tilted at a specified angle (all projected into view space)
create table face (perimeter polygon, c color, z0 real, normal vector);

-- SPOT: A pixel's worth of color at a specified distance from and angle to the viewer
create type spot as (z real, c color, normal vector);
create function the_spot(real,color,vector) returns spot as 'select $1,$2,$3;' language sql;
create function color_of(spot) returns color as 'select $1.c;' language sql;

--
-- Set up the image to render; normally this would come from outside
--
-- TODO: Get real / interesting image data in here!
insert into face values ( '( (-Infinity,-Infinity),(-Infinity,Infinity), (Infinity,Infinity), (Infinity,-Infinity) )',(0.0,0.0,0.0,1.0),10000,(0.0,0.0,1.0));
-- insert into face values ( '( ( 0.0 ,0.0 ),( 0.0,100.0 ), ( 100.0,100.0 ) )',(0.5,0.5,0.0,1.0),0.0,(0.0,0.0,1.0));
insert into face
    select
        polygon(array_to_string(ARRAY[
            2*s*i+s,2*s*j+s,
            2*s*i+s,2*s*j-s,
            2*s*i-s,2*s*j-s,
            2*s*i-s,2*s*j+s],',')),
        case when (i % 2) = (j % 2) then
            the_color(0.8,0.8,0.8)
          else
            the_color(0.2,0.2,0.2)
          end,
        0.0,
        the_vector(0.0,0.0,1.0)
      from
        (select 3.5 as s) const 
      cross join
        (select generate_series(-50,50) as i) i 
      cross join
        (select generate_series(-50,50) as j) j; 
-- TODO: translate the above into image space!

--
-- How far back from the viewer is the spot where the pixel-ray intersects the polygon
--
create function depth_of_intersection(point,face) returns real as $$
    select cast($2.z0 + $2.normal.x*$1[0] + $2.normal.y*$1[1] as real);
    $$ language sql;

--
-- Accumulate colors as we walk the pixel-ray from back to front
--
-- TODO: use alpha and normal to blend, rather than just treating all spots as opaque/matte blobs
create function add_color(spot,spot) returns spot as $$
    select $2;
    $$ language sql;

create aggregate color_from  (
    basetype = spot,
    sfunc = add_color,
    stype = spot,
    finalfunc = color_of,
    initcond = '(Infinity,"(0.0,0.0,0.0,0.0)","(0.0,0.0,-1.0)")'
    );

--
-- Walk through all the pixels we need, accumulating the rgba values
--

-- TODO: Wrap this in an agregator that makes it into a blob w. the image data
--       The priority of this has dropped considerably with the ppm / ruby wrapper
-- TODO: Un-embedd the height and width

select
    color_from(the_spot(z,c,n)) as pixel 
  from 
(select 
    x.x as x, 
    y.y as y, 
    depth_of_intersection(point(x.x,y.y),f) as z,
    f.c as c,
    f.normal as n
  from 
    (select generate_series(1,50) as y) y cross join (select generate_series(1,70) as x) x left join 
  face f on f.perimeter ~ point(x.x,y.y)
  order by y desc,x,z desc
) v
  group by y,x
  order by y,x
  ;


