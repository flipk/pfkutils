
## usage: gplot(v,f)
##   creates file f containing 5 columns:
##       sample_number real imag abs arg
##   this file is suitable for plotting in gnuplot.
##
## example:
##   f1=fir1(300,0.0541666); [h,w]=freqz(f1,1,1000); gplot(h, "freqz");
## then in gnuplot:
##   splot 'freqz' with lines
##   plot 'freqz' using 1:4 with lines

function gplot(v,f)
  if nargin != 2
    print_usage;
  endif
  outf = fopen(f,"w");
  if outf < 0
    printf("failed to open file '%s'\n", f);
    return;
  endif
  for i = 1:length(v)
    v0=v(i);
    fprintf(outf, "%d %e %e %e %e\n", i,
            real(v0), imag(v0), abs(v0), arg(v0));
  endfor
  fclose(outf);
  printf("splot '%s' with lines\n", f);
  printf("plot '%s' using 1:4 with lines\n", f);
  printf("plot '%s' using ($1/%d):(20*log10($4)) with lines\n",
         f, length(v));
endfunction
