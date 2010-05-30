def graph(name,args={})
    args = {
        :range     => '[-2*pi:2*pi]',
        :data      => [['cos(x)','black',1]]
    }.merge(args)
    lines = 0...(args[:data].length)
    File.open('temp.gnuplot','w') { |f| f.print %Q{
        set terminal png transparent nocrop enhanced font courier size 300,200
        set output '#{name}.png'
        #set key inside left vertical Right noreverse enhanced autotitles box linetype rgb "cyan" linewidth 1.000
        set nokey
        set noborder
        set xzeroaxis lt -1
        set noxtics
        set noytics
        #{lines.collect { |i|
            d = args[:data][i]
            "set style line #{i+2} lt rgb '#{d[1]||'black'}'  lw #{d[2]||1} # #{d.inspect}"
        }.join("\n        ")}
        #set border 15 lt rgb "cyan"
        set samples 400, 400
        plot #{args[:range]} #{lines.collect { |i| "#{args[:data][i][0]} ls #{i+2}" }.join(', ')}
    }}
    `/opt/local/bin/gnuplot temp.gnuplot`
end

graph('test',
  :range => '[0:3*pi]',
  :data => [
    ['cos(x*4)','red',2],
    ['sin(x*3)','blue',2],
    ['sin(x*3)+cos(x*4)','purple',3]
  ]
)

graph('sine_x1', :data => [['sin(x)'    ]])
graph('sine_x2', :data => [['sin(1.7*x)']])

graph('sine_x1_plus_sine_x2', 
   :data => [
       ['sin(x)','grey',1],
       ['sin(1.7*x)','grey',1],
       ['sin(x)+sin(1.7*x)']
       ]
)

graph('jello_in',            :data => [['-sin(2*x)']])
graph('compresed_jello_out', :data => [['-0.8*sin(2*x)']])
graph('stretched_jello_out', :data => [['-0.3*sin(2*x)']])

graph('sine_x1_heterodyne_sine_x2', 
   :data => [
       ['sin(x)','grey',1],
       ['sin(1.7*x)','grey',1],
       ['sin(x)+sin(1.7*x)',"black",1],
       ['sin(x)+sin(1.7*x)+sin(x)*sin(1.7*x)',"black",3]
       ]
)

graph('sine_x1_heterodyne_sine_x2_decomposed', 
   :data => [
       ['sin(x)','grey',1],
       ['sin(1.7*x)','grey',1],
       [' 0.5*cos(0.7*x)','blue',1],
       ['-0.5*cos(2.7*x)','red',1],
       ['sin(x)+sin(1.7*x)+sin(x)*sin(1.7*x)',"black",3],
       ]
)
%q{

sin(f1) * sin(f2) = 1/2 ( cos(f1 + f2) - cos(f1-f2) )

-----------

sin(x) + sin(1.7x) = sin(1.7x) - sin(x) + sin(x)sin(1.7x)

yielding:

-sin(x)
sin(1.7x)
cos(.7x)
cos(2.7x)

-----------


}
