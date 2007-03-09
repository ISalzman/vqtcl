# vlerq.py -=- Main Python module for Vlerq

import thrive, os, sys, time

def readfile(f):
  fd = open(f)
  s = fd.read()
  fd.close()
  return s

rcache = {}
def lookup(cmd):
  if not rcache.has_key(cmd):
    try:
      rcache[cmd] = _T.find(cmd)
    except :
      raise '%s: symbol not found' % cmd
  return rcache[cmd]

handler = {
  1: lambda: _T.push(eval(_T.pop())),
  3: lambda: sys.stdout.write(str(_T.pop())),
  4: lambda: _T.push(readfile(_T.pop())),
  5: lambda: _T.push(int(time.time() * 1000000) & 0x7FFFFFFF),
  6: lambda: _T.push(os.getenv(_T.pop())),
}

def vcall(cmd,*args):
  apply(_T.push,args)
  r = _T.run(lookup(cmd))
  while r > 0:
    handler[r]()
    r = _T.run()
  if r < 0:
    return _T.pop()

def vopen(filename):
  return view(vcall('mkfopen',filename))

class row:
  def __init__(self,v,i):
    self._view = v
    self._pos = i
  def __getattr__(self,prop):
    #return self(prop)
    return self._view.at(self._pos,prop)
  def __call__(self,col):
    return self._view.at(self._pos,col)
  def __str__(self):
    return `self`
  def __repr__(self):
    r = ["Row #%d with properties:" % self._pos]
    for p in self._view.names():
      r.append(p)
    return " ".join(r)

class view:
  def __init__(self,r):
    self.ref = r
  def __len__(self):
    return vcall('^count',self.ref)
  def __getitem__(self,index):
    if index >= len(self):
      raise IndexError
    return row(self,index)
  def __str__(self):
    return self.dump()
  def row(self,i):
    return vcall('getrow',self.ref,i)
  def cols(self):
    return vcall('^width',self.ref)
  def col(self,i):
    return vcall('^getcol',self.ref,i)
  def at(self,r,c):
    return view(vcall('getcell',self.ref,r,c))

  def append(self,v):
    return view(vcall('^append',self.ref,v.ref))
  def blocked(self):
    return view(vcall('^blocked',self.ref))
  def clone(self):
    return view(vcall('^clone',self.ref))
  def concat(self,v):
    return view(vcall('^concat',self.ref,v.ref))
  def counts(self,v):
    return vcall('^counts',self.ref,v)
  def delete(self,v,w):
    return view(vcall('^counts',self.ref,v,w))
  def describe(self):
    return view(vcall('^describe',self.ref))
  def divide(self,v):
    return view(vcall('^divide',self.ref,v.ref))
  def except_v(self,v):
    return view(vcall('^except',self.ref,v.ref))
  def first(self,v):
    return view(vcall('^first',self.ref,v))
  def freeze(self):
    return view(vcall('^freeze',self.ref))
  def group(self,*v):
    return view(vcall('^group',self.ref,v[0:-1],v[-1]))
  def insert(self,v,w):
    return view(vcall('^insert',self.ref,v,w.ref))
  def intersect(self,v):
    return view(vcall('^intersect',self.ref,v.ref))
  def join(self,v,w):
    return view(vcall('^join',self.ref,v.ref,w))
  def join0(self,v):
    return view(vcall('^join0',self.ref,v.ref))
  def join_i(self,v):
    return view(vcall('^join_i',self.ref,v.ref))
  def join_l(self,v):
    return view(vcall('^join_l',self.ref,v.ref))
  def last(self,v):
    return view(vcall('^last',self.ref,v))
  def mapcols(self,*v):
    return view(vcall('^mapcols',self.ref,v))
  def mapcols_omit(self,*v):
    return view(vcall('^mapcols',self.ref,('-omit',)+v))
  def maprow(self,v):
    return view(vcall('^maprow',self.ref,v))
  def meta(self):
    return view(vcall('^meta',self.ref))
  def names(self):
    return vcall('^names',self.ref)
  def nspread(self,v):
    return view(vcall('^nspread',self.ref,v))
  def pair(self,v):
    return view(vcall('^pair',self.ref,v.ref))
  def pass_v(self,v):
    return view(vcall('^pass',self.ref,v.ref))
  def pick(self,v):
    return view(vcall('^pick',self.ref,v))
  def product(self,v):
    return view(vcall('^product',self.ref,v.ref))
  def project(self,*v):
    return view(vcall('^project',self.ref,v))
  def project_omit(self,*v):
    return view(vcall('^project',self.ref,('-omit',)+v))
  def rename(self,*v):
    return view(vcall('^rename',self.ref,v))
  def repeat(self,v):
    return view(vcall('^repeat',self.ref,v))
  def replace(self,v,w):
    return view(vcall('^replace',self.ref,v,w.ref))
  def reverse(self):
    return view(vcall('^reverse',self.ref))
  def set(self,v,w,x):
    return view(vcall('^set',self.ref,v,w,x))
  def slice(self,v,w,x):
    return view(vcall('^slice',self.ref,v,w,x))
  def sort(self,*v):
    return view(vcall('^sort',self.ref,v))
  def sortmap(self):
    return vcall('sortmap',self.ref)
  def spread(self,v):
    return view(vcall('^spread',self.ref,v))
  def subcat(self,v):
    return view(vcall('^subcat',self.ref,v))
  def tag(self,v):
    return view(vcall('^tag',self.ref,v))
  def take(self,v):
    return view(vcall('^take',self.ref,v))
  def types(self):
    return vcall('^types',self.ref)
  def ungroup(self,v):
    return view(vcall('^ungroup',self.ref,v))
  def union(self,v):
    return view(vcall('^union',self.ref,v.ref))
  def uniqmap(self):
    return vcall('^uniqmap',self.ref)
  def unique(self):
    return view(vcall('^unique',self.ref))

  def dump(self,maxrows=20,maxwidth=50):
    o = []
    nm = self.names()
    if len(nm) == 0: return
    tp = self.types()
    cv = []
    fmt = []
    hdr = bar = ""
    for i in range(len(nm)):
      c = self.col(i)[:maxrows]
      if tp[i] == 'B':
	c = ['%db' % len(x) for x in c]
      elif tp[i] == 'V':
	c = ['#%d' % vcall('^count',x) for x in c]
      cv.append(c)
      w = len(nm[i])
      for x in cv[-1]:
	w = max(w,len(str(x)))
      w = min(w,maxwidth)
      if tp[i] in 'IBV':
	fmt.append("  %%%ds" % w)
      else:
	fmt.append("  %%-%d.%ds" % (w,w))
      hdr = hdr + "  " + nm[i].ljust(w)
      bar = bar + "  " + ('-' * w)
    o.append(hdr)
    o.append(bar)
    n = len(self)
    r = range(len(cv))
    for i in range(min(n,maxrows)):
      o.append("".join([fmt[j] % cv[j][i] for j in r]))
    if n > maxrows:
      o.append(bar.replace('-','.'))
    return "\n".join(o)

def vdef(*args):
  data = args[-1]
  args = args[:-1]
  c = len(args)
  mdata = []
  desc = ""
  if type(data) == type(0):
    if c != 0:
      raise "empty view, count expected"
    n = data
  else:
    d = len(data)
    if d > 0:
      if c == 0: raise "no args defined"
      if d % c != 0: raise "data not an exact number of rows"
      n = d/c
    else:
      n = 0
    for x in args:
      v = (x+':S').split(':')
      mdata.append(v[0])
      mdata.append(ord(v[1]))
      mdata.append(-1)
  return view(vcall('mkvdef',n,data,mdata))

_T = thrive.workspace()
_T.push(sys.argv)

vqinit = open('words1.th').read() + open('words2.th').read() 

for x in vqinit.split('\n'):
  _T.load(x)
