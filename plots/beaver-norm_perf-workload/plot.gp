reset
set term post eps enhanced dashed color font 'Helvetica,18'

myred = '#941100'
myblue = '#7B8CAB'
mygreen = '#4F8F00'
mygray = 'grey70'
myblack = 'black'

set size 0.5,0.5

set style fill pattern
set style histogram clustered

set xlabel "Workload" offset 0,0.5
set ylabel "Normalized performance" offset 0.5,0

set yrange [0:]
# set format y '%.0f'

set tics out nomirror
set grid ytics
set xtics rotate by 10 offset -2.5 font ",12"

# Gnuplot has 5 distinct coordinate systems: first, second, graph, screen, character
set key at graph 1.0,0.5
set key maxrows 4
set key opaque
set key font ",16"
set key samplen 1

set output "beaver-norm_perf-workload.eps"
plot "data.dat" i 0 u ($2/$2):xtic(1) t "Throughput w/o Beaver" w histograms lc rgb myred fs pattern 1,\
     "" i 0 u ($3/$2):xtic(1) t "Throughput w/ Beaver" w histograms lc rgb myred fs pattern 2,\
     "" i 0 u ($4/$4):xtic(1) t "p99 latency w/o Beaver" w histograms lc rgb myblue fs pattern 1,\
     "" i 0 u ($5/$4):xtic(1) t "p99 latency w/ Beaver" w histograms lc rgb myblue fs pattern 2
