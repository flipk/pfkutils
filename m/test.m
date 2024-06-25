
# first way:
# stage 1 1/5
# f1=fir2(30,[0 0.24 0.30 1],[1 1 0 0]);
# 30 mults every 5th sample
# stage 2 1/10 
# f1=fir2(100,[0 0.09 0.11 1],[1 1 0 0]);

# second way:
# stage 1 1/10
# 40 mults every 10th sample
# f1=fir2(40,[0 0.13 0.13 1],[1 1 0 0]);
# stage 2 1/5
# f1=fir2(100,[0 0.218 0.218 1],[1 1 0 0]);

[h,w]=freqz(f1,1,10000);
gplot(h, 'stuff');
