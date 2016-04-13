
edit a bdf file using gbdfed

bdftopcf -t < 5x7.bdf | gzip > 5x7.pcf.gz

copy it to $HOME/pfk/fonts/

in that dir, run "mkfontdir ." to update fonts.dir

consider editing fonts.alias to make short names

xset +fp $HOME/pfk/fonts
xset fp rehash
