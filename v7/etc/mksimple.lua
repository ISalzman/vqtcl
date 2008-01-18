require "lvq"

v = view{ meta='abc:I'; 1, 22, 333, 4444, 55555 }

v:p()

view{ meta='v[abc:I]'; v }:p():save('simple.db')
