puts "s/=/=%/"
(1..9).to_a.reverse.each { |a|
    puts "g/\\([0-9]*\\)#{a}+\\([0-9]*\\)=/s//\\1#{a-1}+\\2=x/"
    puts "g/\\([0-9]*\\)+\\([0-9]*\\)#{a}=/s//\\1+\\2#{a-1}=x/"
}
puts "g/\\([0-9]*\\)0+\\([0-9]*\\)0=/s//\\1+\\2=%/"
puts "g/=%xxxxxxxxxx/s//=:%/"
puts "g/%xxxxxxxxx%/s//9/"
puts "g/%xxxxxxxx%/s//8/"
puts "g/%xxxxxxx%/s//7/"
puts "g/%xxxxxx%/s//6/"
puts "g/%xxxxx%/s//5/"
puts "g/%xxxx%/s//4/"
puts "g/%xxx%/s//3/"
puts "g/%xx%/s//2/"
puts "g/%x%/s//1/"
puts "g/%%/s//0/"
puts "g/\\([0-9][0-9]*\\)+=/s//\\1/"
puts "g/^+\\([0-9][0-9]*\\)=/s//\\1/"
puts "g/9:/s//:9/"
(1..9).each { |a| puts "g/#{9-a}:/s//#{10-a}/" }
puts "g/^:/s//1/"
puts "w"
puts "g/[=:]/!ed % < sum.ed"
puts "q"

