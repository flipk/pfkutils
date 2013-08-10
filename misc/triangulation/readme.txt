
all units are in meters


emitter 1 :  0,  0,  0
emitter 2 : 10,  0,  0
emitter 3 :  5, 10,  0
emitter 4 :  5,  5, 10

speed of sound: 300 meters/second = 3.333 milliseconds/meter

emission pattern:
 emitter 1 : t=0
 emitter 2 : t=100ms
 emitter 3 : t=200ms
 emitter 4 : t=300ms

distance calculation for (x1,y1,z1) to (x2,y2,z2) :

d = sqrt( (x1-x2)^2 + (y1-y2)^2 + (z1-z2)^2 )


sample points for consideration:

   p1 = (0,10,0)
   p2 = (5,10,10)
   p3 = (5,0,5)

