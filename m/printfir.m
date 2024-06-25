
## usage: printfir(n,w,f)
##  call fir1(n,w) and print results in { %f, %f } format to
##  file f for inclusion in C code
##
## example:
##   printfir(100,0.02,"filter_02.h");

function printfir(n,w,f)
  if nargin != 3
    print_usage;
  endif
  f1 = fir1(n,w);
  out_f = fopen(f,"w");
  fprintf(out_f, "// generate in octave using: fir1(%d,%f)\n", n,w);
  fprintf(out_f, "{\n\t");
  line_count=0;
  for i=1:n
    fprintf(out_f, "%.9e, ", f1(i));
    line_count++;
    if line_count == 4
      fprintf(out_f, "\n\t");
      line_count=0;
    endif
  endfor
  fprintf(out_f, "%.9e\n}\n", f1(n+1));
  fclose(out_f);
endfunction
