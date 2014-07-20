from __future__ import print_function, division, absolute_import

def boxgrow(b,r):
    return ((b[0][0]-r,b[0][1]+r),
            (b[1][0]-r,b[1][1]+r),
            (b[2][0]-r,b[2][1]+r))

def boxjoin(a,b):
    return ((min(a[0][0],b[0][0]),max(a[0][1],b[0][1])),
           (min(a[1][0],b[1][0]),max(a[1][1],b[1][1])),
           (min(a[2][0],b[2][0]),max(a[2][1],b[2][1])))

def boundboxv(vtxl):
    x1,x2,y1,y2,z1,z2 = 0,0,0,0,0,0
    mn = min
    mx = max
    for x,y,z in vtxl:
        x1 = mn(x1,x)
        x2 = mx(x2,x)
        y1 = mn(y1,y)
        y2 = mx(y2,y)
        z1 = mn(z1,z)
        z2 = mx(z2,z)
    return ((x1,x2),(y1,y2),(z1,z2))

def boxcenter(b):
    return (((b[0][0]+b[0][1])/2.0),
            ((b[1][0]+b[1][1])/2.0),
            ((b[2][0]+b[2][1])/2.0))

def boxsize(b):
    return ((b[0][1]-b[0][0]),
            (b[1][1]-b[1][0]),
            (b[2][1]-b[2][0]))
