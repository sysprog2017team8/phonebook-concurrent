reset
set ylabel 'time(sec)'
set style fill solid
set title 'average Append / Remove time'
set term png enhanced font 'Verdana,10'
set output 'average_Append_Remove_time.png'

plot [:3][:] 'test.txt' using 2:xtic(1) with histogram title 'linked-list' , \
'' using ($0-0.12):($2+0.001):2 with labels title ' ' , \
'' using 3:xtic(1) with histogram title 'concurrent linked-list(lock)' , \
'' using ($0+0.06):($3+0.001):3 with labels title ' ' , \
'' using 4:xtic(1) with histogram title 'concurrent linked-list(lockfree)' ,\
'' using ($0+0.24):($4+0.001):4 with labels title ' ' , \
