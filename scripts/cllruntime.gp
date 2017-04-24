reset
set ylabel 'time(sec)'
set style fill solid
set title 'perfomance comparison'
set term png enhanced font 'Verdana,10'
set output 'cllruntime.png'

plot [:][:]'output.txt' using 2:xtic(1) with histogram title 'linked-list', \
'' using 3:xtic(1) with histogram title 'concurent lock'  , \
'' using 4:xtic(1) with histogram title 'concurent lockfree'  , \
'' using ($0-0.36):($2+0.002):2 with labels title '', \
'' using ($0+0.0):($3+0.05):3 with labels title '', \
'' using ($0+0.3):($4+0.1):4 with labels title ''
