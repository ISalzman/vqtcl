--[[  Mutable view tests for LuaVlerq, called from test.lua wrapper.
      $Id$
      This file is part of Vlerq, see src/vlerq.h for full copyright notice.
--]]

-- set up a nested view to test mutable layer with
local vo = vq{meta='A,X[B,C]'; 11, vq{meta='B,C'; 1211,1212,1221,1222},
                               21, vq{meta='B,C'},
                               31, vq{meta='B,C'; 3211,3212}}
                               
-- convert nested view to a read-only mapped one
local ve = vo:emit()
assert(#ve == 118, 've as bytes')
local vr = vq.load(ve)

assert(vr:s() == 'A X; 11 #2; 21 #0; 31 #1', 'vr input')
assert(vr[1].X:s() == 'B C; 1211 1212; 1221 1222', 'vr sub 1')
assert(vr[2].X:s() == 'B C', 'vr sub 2')
assert(vr[3].X:s() == 'B C; 3211 3212', 'vr sub 3')

local vm = vr:mutwrap()
vm[1].A = 'a'
assert(vr:s() == 'A X; 11 #2; 21 #0; 31 #1', 'vr unaffected')
assert(vm:s() == 'A X; a #2; 21 #0; 31 #1', 'vm changed')

-- vm[1].X[1].B = 'b'
-- assert(vr[1].X:s() == 'B C; 1211 1212; 1221 1222', 'vr sub unaffected')
-- assert(vm[1].X:s() == 'B C; b 1212; 1221 1222', 'vm sub changed')

print "End tmutable"
