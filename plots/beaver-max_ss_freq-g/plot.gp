reset
set term post eps enhanced color font 'Helvetica,22'

myred = '#941100'
myblue = '#7B8CAB'
mygreen = '#4F8F00'
mygray = 'grey70'
myblack = 'black'

# set xlabel "|P^{in}|" offset 0,0.5
set xlabel "|G|" offset 0,0.5
set ylabel "Snapshot frequency [Hz]" offset 0.5,0

# set logscale y 10
# set format y "10^{%L}"
# set format y "%.1tx10^{%T}"
set yrange [0:]

set size 0.65,0.65

set border 3
unset key
unset grid
set mxtics 1
set tics out nomirror
set xtics rotate by 10 offset -1.5 font ",20"
set ytics font ",20"

set grid ytics

set bars 3
set boxwidth 0.8 relative

set output "beaver-ss_freq-g_wopara.eps"
plot "data.dat" using 1:2:xtic(sprintf("%d", $1)) with boxes fill pattern 1 ls 1 lw 3 lc rgb myred
set output "beaver-ss_freq-g_wpara.eps"
plot "data.dat" using 1:3:xtic(sprintf("%d", $1)) with boxes fill pattern 3 ls 1 lw 3 lc rgb myblue
