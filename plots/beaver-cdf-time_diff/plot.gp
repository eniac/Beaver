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

set key at graph 1.0,0.6
set key font ",14"
# set key opaque
set key enhanced
set key maxrows 3
set key samplen 1

set xlabel "Time difference [{/Symbol m}s]"
set ylabel "CDF" offset 1,0
set size 0.5,0.5
set tics nomirror
set key spacing 1.5
set border 3

set output "beaver-cdf-time_diff-g16.eps"
plot "data.dat" i 0 using ($2/1000):($1/100) title "t_{1}-t_{0}" with lines linetype 1 linewidth 6 linecolor rgbcolor myred, \
     "data.dat" i 0 using ($3/1000):($1/100) title "e@^{ss}_{gmax}.t - e@^{ss}_{gmin}.t" with lines linetype 1 linewidth 6 linecolor rgbcolor myblue, \
     "data.dat" i 0 using ($4/1000):($1/100) title "(t_{1}-t_{0})-(e@^{ss}_{gmax}.t - e@^{ss}_{gmin}.t)" with lines linetype 1 linewidth 6 linecolor rgbcolor mygreen

set output "beaver-cdf-time_diff-g8.eps"
plot "data.dat" i 1 using ($2/1000):($1/100) title "t_{1}-t_{0}" with lines linetype 1 linewidth 6 linecolor rgbcolor myred, \
     "data.dat" i 1 using ($3/1000):($1/100) title "e@^{ss}_{gmax}.t - e@^{ss}_{gmin}.t" with lines linetype 1 linewidth 6 linecolor rgbcolor myblue, \
     "data.dat" i 1 using ($4/1000):($1/100) title "(t_{1}-t_{0})-(e@^{ss}_{gmax}.t - e@^{ss}_{gmin}.t)" with lines linetype 1 linewidth 6 linecolor rgbcolor mygreen

set output "beaver-cdf-time_diff-g4.eps"
plot "data.dat" i 2 using ($2/1000):($1/100) title "t_{1}-t_{0}" with lines linetype 1 linewidth 6 linecolor rgbcolor myred, \
     "data.dat" i 2 using ($3/1000):($1/100) title "e@^{ss}_{gmax}.t - e@^{ss}_{gmin}.t" with lines linetype 1 linewidth 6 linecolor rgbcolor myblue, \
     "data.dat" i 2 using ($4/1000):($1/100) title "(t_{1}-t_{0})-(e@^{ss}_{gmax}.t - e@^{ss}_{gmin}.t)" with lines linetype 1 linewidth 6 linecolor rgbcolor mygreen

set output "beaver-cdf-time_diff-g2.eps"
plot "data.dat" i 3 using ($2/1000):($1/100) title "t_{1}-t_{0}" with lines linetype 1 linewidth 6 linecolor rgbcolor myred, \
     "data.dat" i 3 using ($3/1000):($1/100) title "e@^{ss}_{gmax}.t - e@^{ss}_{gmin}.t" with lines linetype 1 linewidth 6 linecolor rgbcolor myblue, \
     "data.dat" i 3 using ($4/1000):($1/100) title "(t_{1}-t_{0})-(e@^{ss}_{gmax}.t - e@^{ss}_{gmin}.t)" with lines linetype 1 linewidth 6 linecolor rgbcolor mygreen