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
-- Define a few constants (syntax not standardized until 2016)
--
WIDTH=500
HEIGHT=350
DEPTH=440
    -- Actually inverse DEPTH; larger values make the image less sensitive to y 
TILT=0.7
SCALE=(<HEIGHT>+<WIDTH>)/2

--
-- Define our types, tables, and helper functions
--

-- COLOR:
create type color as (r real, g real, b real, a real);
create or replace function the_color(real,real,real)      returns color as 'select $1,$2,$3,$1-$1;' language sql;
create or replace function the_color(real,real,real,real) returns color as 'select $1,$2,$3,$4;'    language sql;

-- VECTOR:
create type vector as (x real, y real, z real);
create or replace function the_vector(real,real,real) returns vector as $$
    select $1,$2,$3;
    $$ language sql;
create or replace function dot(vector,vector) returns real as $$
    select $1.x*$2.x + $1.y*$2.y + $1.z*$2.z;
    $$ language sql;

-- FACE: A colored polygon at a given distance & tilted at a specified angle (all projected into view space)
create table face (perimeter polygon, c color, z0 real, normal vector);

-- SPOT: A pixel's worth of color at a specified distance from and angle to the viewer
create type spot as (z real, c color, normal vector);
create or replace function the_spot(real,color,vector) returns spot as 'select $1,$2,$3;' language sql;
create or replace function color_of(spot) returns color as 'select $1.c;' language sql;

--
-- Set up the image to render; normally this would come from outside
--
-- TODO: This way of projecting still sort of sucks
-- TODO: Fix the normal / root intersection computation
--
create or replace function image_x(vector) returns real as $$
     select cast(<WIDTH>/2 + $1.x*<DEPTH>/(<DEPTH>+$1.y) as real);
  $$ language sql;

create or replace function image_y(vector) returns real as $$
     select cast(<HEIGHT>-($1.y*<TILT>+$1.z)*<DEPTH>/(<DEPTH>+$1.y) as real);
  $$ language sql;

create or replace function image_xy(vector) returns text as $$
    select
        to_char(image_x($1),'999999.99') || ',' || to_char(image_y($1),'999999.99');
  $$ language sql;

create or replace function image_xy(x real, y real, z real) returns text as $$
    select image_xy(the_vector($1,$2,$3));
  $$ language sql;

MAX_J=20
insert into face
    select
        polygon(array_to_string(ARRAY[
            image_xy(2*s*i+s,2*s*j+s,0),
            image_xy(2*s*i+s,2*s*j-s,0),
            image_xy(2*s*i-s,2*s*j-s,0),
            image_xy(2*s*i-s,2*s*j+s,0)],',')),
        case when ((i+1000) % 2) = ((j+1000) % 2) then
            the_color(0.8,0.7,0.7+(0.3*j)/<MAX_J>)
          else
            the_color(0.1+(0.4*j)/<MAX_J>,0.0+(0.5*j)/<MAX_J>,0.1+(0.6*j)/<MAX_J>)
          end,
        0.0,
        the_vector(0.0,0.0,1.0)
      from
        (select 0.2*<SCALE> as s) const 
      cross join
        (select generate_series(-15,15) as i) i 
      cross join
        (select generate_series(0,<MAX_J>) as j) j; 


-- TODO: This feels like it ought to work, but it doesn't, so we add the faces individually
/*
insert into face
    select
        polygon(array_to_string(ARRAY[
            image_xy(p1),
            image_xy(p2),
            image_xy(p3),
            image_xy(p4)],',')),
        the_color(0.8,0.8,0.0,0.5),
        0.0,
        the_vector(0.0,0.0,1.0)
      from
        (select
            the_vector( 100.0, 300.0,   0.0) as p1,
            the_vector(-100.0, 100.0,   0.0) as p2,
            the_vector(-100.0, 100.0, 282.0) as p3,
            the_vector( 100.0, 300.0, 282.0) as p4
          union select 
            the_vector(-100.0, 100.0,   0.0) as p1,
            the_vector(-300.0, 300.0,   0.0) as p2,
            the_vector(-300.0, 300.0, 282.0) as p3,
            the_vector(-100.0, 100.0, 282.0) as p4
          union select
            the_vector(-100.0, 100.0, 282.0) as p1,
            the_vector(-300.0, 300.0, 282.0) as p2,
            the_vector(-100.0, 500.0, 282.0) as p3,
            the_vector( 100.0, 300.0, 282.0) as p4
          ) jello_faces;
*/
insert into face
    select
        polygon(array_to_string(ARRAY[
            image_xy(p1),
            image_xy(p2),
            image_xy(p3),
            image_xy(p4)],',')),
        the_color(0.8,0.8,0.0,0.5),
        -1.0,
        the_vector(0.0,0.0,1.0)
      from
        (select
            the_vector( 100.0, 300.0,   0.0) as p1,
            the_vector(-100.0, 100.0,   0.0) as p2,
            the_vector(-100.0, 100.0, 282.0) as p3,
            the_vector( 100.0, 300.0, 282.0) as p4
          ) jello_faces;


insert into face
    select
        polygon(array_to_string(ARRAY[
            image_xy(p1),
            image_xy(p2),
            image_xy(p3),
            image_xy(p4)],',')),
        the_color(0.8,0.8,0.0,0.5),
        -1.0,
        the_vector(0.0,0.0,1.0)
      from
        (select
            the_vector(-100.0, 100.0,   0.0) as p1,
            the_vector(-300.0, 300.0,   0.0) as p2,
            the_vector(-300.0, 300.0, 282.0) as p3,
            the_vector(-100.0, 100.0, 282.0) as p4
          ) jello_faces;


insert into face
    select
        polygon(array_to_string(ARRAY[
            image_xy(p1),
            image_xy(p2),
            image_xy(p3),
            image_xy(p4)],',')),
        the_color(0.8,0.8,0.0,0.5),
        -1.0,
        the_vector(0.0,0.0,1.0)
      from
        (select
            the_vector(-100.0, 100.0, 282.0) as p1,
            the_vector(-300.0, 300.0, 282.0) as p2,
            the_vector(-100.0, 500.0, 282.0) as p3,
            the_vector( 100.0, 300.0, 282.0) as p4
          ) jello_faces;



--
-- Translate the above into image space!
--

-- 
-- Finally, put a lot of blue sky out there in the background
--
insert into face values ( '( (-Infinity,-Infinity),(-Infinity,Infinity), (Infinity,Infinity), (Infinity,-Infinity) )',(0.6,0.5,1.0,0.0),10000,(0.0,0.0,1.0));
--
-- How far back from the viewer is the spot where the pixel-ray intersects the polygon
--
create or replace function depth_of_intersection(point,face) returns real as $$
    select cast($2.z0 + $2.normal.x*$1[0] + $2.normal.y*$1[1] as real);
    $$ language sql;

--
-- Accumulate colors as we walk the pixel-ray from back to front
--
-- TODO: use delta_depth to scale alpha
create or replace function add_color(spot,spot) returns spot as $$
    select 
        the_spot(
            $2.z,
            the_color(
                cast(($1.c.r*$2.c.a+$2.c.r*(1.0-$2.c.a)) as real),
                cast(($1.c.g*$2.c.a+$2.c.g*(1.0-$2.c.a)) as real),
                cast(($1.c.b*$2.c.a+$2.c.b*(1.0-$2.c.a)) as real)
            ),
            $2.normal
        );
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
    (select generate_series(1,<HEIGHT>) as y) y 
  cross join 
    (select generate_series(1,<WIDTH>) as x) x 
  left join 
    face f on f.perimeter ~ point(x.x,y.y)
  order by y desc,x,z desc
) v
  group by y,x
  order by y,x
  ;


