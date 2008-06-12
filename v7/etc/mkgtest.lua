package.cpath = '../src/?.so;' .. package.cpath
package.path = '../src/?.lua;' .. package.path
require "vq"

-- dataset adopted from GadFly

frequents = { meta='drinker,bar,perweek:I';
  'adam', 'lolas', 1, 
  'woody', 'cheers', 5, 
  'sam', 'cheers', 5, 
  'norm', 'cheers', 3, 
  'wilt', 'joes', 2, 
  'norm', 'joes', 1, 
  'lola', 'lolas', 6, 
  'norm', 'lolas', 2, 
  'woody', 'lolas', 1, 
  'pierre', 'frankies', 0 }

serves = { meta='bar,beer,quantity:I';
  'cheers', 'bud', 500,
  'cheers', 'samaddams', 255,
  'joes', 'bud', 217,
  'joes', 'samaddams', 13,
  'joes', 'mickies', 2222,
  'lolas', 'mickies', 1515,
  'lolas', 'pabst', 333,
  'winkos', 'rollingrock', 432,
  'frankies', 'snafu', 5 }

likes = { meta='drinker,beer,perday:I';
  'adam', 'bud', 2,
  'wilt', 'rollingrock', 1,
  'sam', 'bud', 2,
  'norm', 'rollingrock', 3,
  'norm', 'bud', 2,
  'nan', 'sierranevada', 1,
  'woody', 'pabst', 2,
  'lola', 'mickies', 5 }

vq(frequents):p()
vq(serves):p()
vq(likes):p()

gdata = vq{ meta = string.format('frequents[%s],serves[%s],likes[%s]',
                                    frequents.meta, serves.meta, likes.meta);
  vq(frequents), vq(serves), vq(likes)
}

gdata:p():save('gdata.db')
