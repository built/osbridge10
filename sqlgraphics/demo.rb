
constants = {}
src = File.read('stab1.sql').
    gsub(/^([A-Z_]+)=(.*)$/) { constants[$1] = $2; '' }.
    gsub(/<([A-Z_]+)>/) { constants[$1] }.
    gsub(/<([A-Z_]+)>/) { constants[$1] }.
    gsub(/<([A-Z_]+)>/) { constants[$1] }.
    gsub(/<([A-Z_]+)>/) { constants[$1] }.
    gsub(/<([A-Z_]+)>/) { constants[$1] }
results = IO.popen("/sw/bin/psql",'r+') { |sql|
    sql.print src
    sql.close_write
    sql.read
    }
h = Integer(constants['HEIGHT'])
w = Integer(constants['WIDTH'])
File.open('demo.ppm','w') { |f|
    f.puts "P3"
    f.puts "#{w} #{h}"
    f.puts "255"
    i = 0
    results.grep(/\((.+),(.+),(.+),(.+)\)/) {
        r,g,b = *[$1,$2,$3].collect { |c| (Float(c)*255).round }
        f.print "#{r} #{g} #{b} "
        f.puts if (i+=1) % w == 0
    }
    puts "Read #{i} pixels."
}
`/sw/bin/ppmtojpeg demo.ppm > demo.jpg`

