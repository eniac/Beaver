reset
set term post eps enhanced dashed color font 'Helvetica,22'

myred = '#941100'
myblue = '#7B8CAB'
mygreen = '#4F8F00'
mygray = 'grey70'
myblack = 'black'

set size 1,0.618

set style fill pattern
set style histogram clustered

set xlabel "Snapshot frequency [Hz]" offset 0,0.5
set ylabel "Effective snapshot rate [%]" offset 0.5,0

set yrange [0:100]
# set format y '%.0f'

set tics out nomirror
set grid ytics
set xtics rotate by 10 offset -1.5 font ",20"

# Gnuplot has 5 distinct coordinate systems: first, second, graph, screen, character
set key at graph 0.3,0.6
set key maxrows 4
set key opaque

set label "Not achievable" at 7.3,graph 0.35 rotate by 90 center font ",20"

set output "beaver-effective_ss_rate-ss_freq.eps"
plot "data.dat" i 0 u 2:xtic(1) t "2" w histograms lc rgb myred fs pattern 1,\
     "" i 0 u 3:xtic(1) t "4" w histograms lc rgb myblue fs pattern 2,\
     "" i 0 u 5:xtic(1) t "8" w histograms lc rgb mygray fs pattern 3,\
     "" i 0 u 9:xtic(1) t "16" w histograms lc rgb mygreen fs pattern 4
