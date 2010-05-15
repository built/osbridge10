h = 150
w = 200
File.open('demo.ppm','w') { |f|
    f.puts "P3"
    f.puts "#{w} #{h}"
    f.puts "255"
    i = 0
    `/sw/bin/psql < stab1.sql`.grep(/\((.+),(.+),(.+),(.+)\)/) {
        r,g,b = *[$1,$2,$3].collect { |c| (Float(c)*255).round }
        f.print "#{r} #{g} #{b} "
        f.puts if (i+=1) % w == 0
    }
    puts "Read #{i} pixels."
}
`/sw/bin/ppmtojpeg demo.ppm > demo.jpg`

