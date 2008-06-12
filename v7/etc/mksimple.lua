require "vq"

v = vq{ meta='abc:I'; 1, 22, 333, 4444, 55555 }

v:p()

vq{ meta='v[abc:I]'; v }:p():save('simple.db')
