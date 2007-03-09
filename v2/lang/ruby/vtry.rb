#! /usr/bin/env ruby
# vtry.rb -=- Try Vlerq from Ruby

require 'thrive'
w = Thrive::Workspace.new
p w
w.load(' 0 exit exec -1 syst [[ ')
p w.find('+')
p w.eval('54321')
p w.run
p w.pop
w.push(123,456)
p w.eval('+')
p w.run
p w.pop
w.push(12,34)
p w.run(w.find('+'))
p w.pop
w.push([1,nil,'a'])
p w.pop

__END__
Output:

#<Thrive::Workspace:0xb7d3e278>
#<Thrive::Reference:0xb7d3e228>
149
0
54321
33
0
579
-1
46
[1, nil, "a"]
