reset
set ylabel 'time(sec)'
set xlabel 'run order'
set style data lines
set title 'All Append Remove run time distribute'
set term png enhanced font 'Verdana,10'
set output 'All_Append_Remove_rumtime.png'

plot [:][:] 'test.txt' using 1:2 with linespoints linewidth 2 title 'linked-list Append()', \
'' using 1:3 with linespoints linewidth 2 title 'linked-list Remove()', \
'' using 1:4 with linespoints linewidth 2 title 'lock Append()', \
'' using 1:5 with linespoints linewidth 2 title 'lock Remove()', \
'' using 1:6 with linespoints linewidth 2 title 'lock-free Append()', \
'' using 1:7 with linespoints linewidth 2 title 'lock-free Remove()'
