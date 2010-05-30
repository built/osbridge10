def graph(name,args={})
    args = {
        :range     => '[0:2*pi]',
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

