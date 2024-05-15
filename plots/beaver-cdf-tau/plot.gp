reset
set term post eps enhanced dashed color font 'Helvetica,22'

myred = '#941100'
myblue = '#7B8CAB'
mygreen = '#4F8F00'
mygray = 'grey50'
myblack = 'black'

set style line 1 linetype 1 linewidth 5 pointtype 4 pointsize 2 linecolor rgbcolor myred # primary line
set style line 2 lt 1 lw 5 pt 2 ps 1.5 lc rgb myblue 
set style line 3 lt 1 lw 5 pt 8 ps 1.5 lc rgb mygreen

set logscale x 10
set format x "10^{%L}"
set yrange [0:1]
set grid

set key at graph 0.6,0.4
set key font ",22"
set key opaque
set key maxrows 3
# set key samplen 1

set xlabel "Time window [{/Symbol m}s]"
set size 1,0.618
# set size 0.5,0.5
set tics nomirror
set border 3

set output "beaver-cdf-tau.eps"
set arrow from 32,graph 0 to 32,graph 1 nohead linetype 2 dashtype 2 linewidth 6 linecolor mygray
set label "{/Symbol t}_{min}" at 20,graph 0.9 center font ",20"
plot "data.dat" i 0 using ($1/1000):2 title "Intra-DC" with lines linetype 1 linewidth 6 linecolor rgbcolor myred, \
                    "" i 1 using ($1/1000):2 title "Inter-DC" with lines linetype 1 linewidth 6 linecolor rgbcolor myblue, \
                    "" i 2 using ($1/1000):2 title "Internet" with lines linetype 1 linewidth 6 linecolor rgbcolor mygreen
