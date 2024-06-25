
## usage: printcarray(v,f)
##  print v in { %f, %f } format to
##  file f for inclusion in C code
##
## example:
##   printcarray([1 2 3], "stuff.h");

function printcarray(v,f)
  if nargin != 2
    print_usage;
  endif
  out_f = fopen(f,"w");
  fprintf(out_f, "{\n\t");
  line_count=0;
  for i=1:(length(v)-1)
    fprintf(out_f, "%.9e, ", v(i));
    line_count++;
    if line_count == 4
      fprintf(out_f, "\n\t");
      line_count=0;
    endif
  endfor
  fprintf(out_f, "%.9e\n}\n", v(length(v)));
  fclose(out_f);
endfunction
