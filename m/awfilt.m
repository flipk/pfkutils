pkg load signal

f02_1000=fir1(1000,1/50);
[h02_1000,w]=freqz(f02_1000,1,10000);
printcarray(f02_1000, "f02_1000.h");

f027_1000=fir1(1000,13/480);
[h027_1000,w]=freqz(f027_1000,1,10000);
printcarray(f027_1000, "f027_1000.h");

f054_1000=fir1(1000,13/240);
[h054_1000,w]=freqz(f054_1000,1,10000);
printcarray(f054_1000, "f054_1000.h");

f02_300=fir1(300,1/50);
[h02_300,w]=freqz(f02_300,1,10000);
printcarray(f02_300, "f02_300.h");

f027_300=fir1(300,13/480);
[h027_300,w]=freqz(f027_300,1,10000);
printcarray(f027_300, "f027_300.h");

f054_300=fir1(300,13/240);
[h054_300,w]=freqz(f054_300,1,10000);
printcarray(f054_300, "f054_300.h");

f369_300=fir1(300,24/65);
[h369_300,w]=freqz(f369_300,1,10000);
printcarray(f369_300, "f369_300.h");

f5_300=fir1(300,0.5);
[h5_300,w]=freqz(f5_300,1,10000);
printcarray(f5_300, "f5_300.h");

save awfilt.save f02_1000 h02_1000 f027_1000 h027_1000 f054_1000 h054_1000 f02_300 h02_300 f027_300 h027_300 f054_300 h054_300 f369_300 h369_300 f5_300 h5_300

