#!/usr/bin/env python

import loos
import loos.pyloos
import sys
import math

if len(sys.argv) < 4 or sys.argv[1] == "-h" or sys.argv[1] == "--help":
    print "Usage: ", sys.argv[0], " system trajectory selection1 [selection2...]"
    print "       Prints the average of the orientation vectors of the "
    print "       individual selections, assuming each individual vector "
    print "       points in the +z direction"
    sys.exit()

#print len(sys.argv)
print "#", " ".join(sys.argv)
system_filename = sys.argv[1]
traj_filename = sys.argv[2]
selections = sys.argv[3:]


system = loos.createSystem(system_filename)
traj = loos.pyloos.Trajectory(traj_filename, system)

helices = []
for s in selections:
    helices.append(loos.selectAtoms(system, s))



print "#Frame\tAngle\tCosine"

for frame in traj:

    vec = loos.GCoord(0.,0.,0.)
    for h in helices:
        pca = h.principalAxes()
        v = pca[0]
        if v.z() < 0:
            v *= -1.
        vec += v

    cosine = vec.z() / vec.length()
    
    cosine = max(-1.0, cosine)
    cosine = min(1.0, cosine)
    ang = math.acos(cosine) * 180./math.pi
    
    print traj.index(), ang, cosine

    
