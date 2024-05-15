reset
set term post eps enhanced dashed color font 'Helvetica,20'

myred = '#941100'
myblue = '#7B8CAB'
mygreen = '#4F8F00'
mygray = 'grey50'
myblack = 'black'

set style line 1 linetype 1 linewidth 5 pointtype 4 pointsize 2 linecolor rgbcolor myred # primary line
set style line 2 lt 1 lw 5 pt 2 ps 1.5 lc rgb myblue 
set style line 3 lt 1 lw 5 pt 8 ps 1.5 lc rgb mygreen

set logscale y 10
set format y "10^{%L}"
# set yrange [0:1]
set xrange [0:1000]
set grid

set key at graph 1.0,0.35
set key font ",12"
# set key opaque
set key enhanced
set key maxrows 3
set key samplen 1
set key spacing 1.5
unset key

set xlabel "Snapshot ID"
set ylabel "Time difference [{/Symbol m}s]"
set size 0.5,0.5
set tics nomirror
set border 3

set output "beaver-time_diff-ss_id-g2.eps"
plot     "65536_2_10000000_visualization.dat" i 0 using 1:($2/1000) title "t_{1}-t_{0}" with lines linetype 1 linewidth 2 linecolor rgbcolor myred,\
        "65536_2_10000000_visualization.dat" i 0 using 1:($3/1000) title "e@^{ss}_{gmax}.t - e@^{ss}_{gmin}.t" with lines linetype 1 linewidth 2 linecolor rgbcolor myblue,\
        "65536_2_10000000_visualization.dat" i 0 using 1:(($2-$3)/1000) title "(t_{1}-t_{0})-(e@^{ss}_{gmax}.t - e@^{ss}_{gmin}.t)" with lines linetype 1 linewidth 2 linecolor rgbcolor mygreen


set output "beaver-time_diff-ss_id-g4.eps"
plot     "65536_4_10000000_visualization.dat" i 0 using 1:($2/1000) title "t_{1}-t_{0}" with lines linetype 1 linewidth 2 linecolor rgbcolor myred,\
        "65536_4_10000000_visualization.dat" i 0 using 1:($3/1000) title "e@^{ss}_{gmax}.t - e@^{ss}_{gmin}.t" with lines linetype 1 linewidth 2 linecolor rgbcolor myblue,\
        "65536_4_10000000_visualization.dat" i 0 using 1:(($2-$3)/1000) title "(t_{1}-t_{0})-(e@^{ss}_{gmax}.t - e@^{ss}_{gmin}.t)" with lines linetype 1 linewidth 2 linecolor rgbcolor mygreen

set output "beaver-time_diff-ss_id-g8.eps"
plot     "65536_8_10000000_visualization.dat" i 0 using 1:($2/1000) title "t_{1}-t_{0}" with lines linetype 1 linewidth 2 linecolor rgbcolor myred,\
        "65536_8_10000000_visualization.dat" i 0 using 1:($3/1000) title "e@^{ss}_{gmax}.t - e@^{ss}_{gmin}.t" with lines linetype 1 linewidth 2 linecolor rgbcolor myblue,\
        "65536_8_10000000_visualization.dat" i 0 using 1:(($2-$3)/1000) title "(t_{1}-t_{0})-(e@^{ss}_{gmax}.t - e@^{ss}_{gmin}.t)" with lines linetype 1 linewidth 2 linecolor rgbcolor mygreen

set output "beaver-time_diff-ss_id-g16.eps"
plot     "65536_16_10000000_visualization.dat" i 0 using 1:($2/1000) title "t_{1}-t_{0}" with lines linetype 1 linewidth 2 linecolor rgbcolor myred,\
        "65536_16_10000000_visualization.dat" i 0 using 1:($3/1000) title "e@^{ss}_{gmax}.t - e@^{ss}_{gmin}.t" with lines linetype 1 linewidth 2 linecolor rgbcolor myblue,\
        "65536_16_10000000_visualization.dat" i 0 using 1:(($2-$3)/1000) title "(t_{1}-t_{0})-(e@^{ss}_{gmax}.t - e@^{ss}_{gmin}.t)" with lines linetype 1 linewidth 2 linecolor rgbcolor mygreen