#!/bin/sh
if [ $1 ] ; then
    CC=$1
else
    CC="origin"
fi

./src/salsify/salsify-receiver 9090 1280 720 >log/recv.log 2>&1 &

nohup ffmpeg -re -i /home/librah/Videos/demo.mp4 -f v4l2 -s 1280x720 /dev/video0 &

sleep 2s

mm-delay 20 mm-link /usr/share/mahimahi/traces/Verizon-LTE-short.up /usr/share/mahimahi/traces/Verizon-LTE-short.down --meter-uplink --meter-uplink-delay -- sh -c "./src/salsify/salsify-sender -d /dev/video0 -p YU12 100.64.0.1  9090 1337 -r ${CC}" >log/sender.log 2>&1 &
