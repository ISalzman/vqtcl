#! /usr/bin/env python
# vtry.py -=- Try some view operations

import vlerq
vdef = vlerq.vdef

print vdef('A','B:I','C',[1,2,3,11,22,33,111,222,333])

R = vdef('A','B','C','a b c d a f c b d'.split())
S = vdef('D','E','F','b g a d a f'.split())
T = vdef('A','B','C','D',
	    'a b c d a b e f b c e f e d c d e d e f a b d e'.split())
U = vdef('C','D','E','c d e c d f d e f'.split())
V = vdef('C','D','c d e f'.split())
W = vdef('A','B','C','1 2 3 a b c'.split())

def show(e):
  print e
  print eval(e)

show('R')
show('S')
show('T')
show('U')
show('V')
show('W')
show('T.maprow([0,1,5])')
show('T.mapcols("A","B")')
show('R.concat(W)')
show('W.pair(S)')
show('R.repeat(3)')
show('R.spread(2)')
show('T.slice(1,3,2)')
show('R.reverse()')
show('R.first(2)')
show('R.first(99)')
show('R.last(2)')
show('R.last(99)')
show('R.tag("N")')
show('vdef(5).tag("N")')
show('R.product(S)')
show('R.tag("N").reverse()')
show('T.maprow(T.sortmap())')
show('T.sort()')
show('R.union(W)')
show('T.project("A","B")')
show('R.intersect(W)')
show('R.except_v(W)')
show('T.pick([0,1,0,1,1,0])')
show('R.nspread([3,2,1])')
show('T.mapcols_omit("C")')
show('T.rename(1,"BB")')
show('T.rename("B","BBB")')

show('T.group(["C","D"],"G")')
#show('T.group(["C","D"],"G").sub(0,2)')
#show('T.group(["C","D"],"G")[0].G')
for r in T.group(["C","D"],"G"):
  print 'row %d, G:' % r._pos
  print r.G
show('T.group(["C","D"],"G").ungroup("G")')
show('T.group(["C"],"H")')
show('T.group(["C"],"H").ungroup("H")')

show('T.join(U,"J")')
#show('T.join(U,"J",["C","D"]).sub(0,"J")')
#show('T.join(U,"J",["C","D"])[0].J')
for r in T.join(U,"J"):
  print 'row %d, J:' % r._pos
  print r.J
show('T.join(U,"J").ungroup("J")')
show('T.join0(U)')
show('T.join_i(U)')
show('T.join_l(U)')
show('T.divide(V)')

#show('vlerq.vopen("/home/jcw/bin/kitten")[0].dirs.mapcols(1,0).sort()')
show('vlerq.vopen("../vqx")[0].dirs[0].files.mapcols(1,0).sort()')
