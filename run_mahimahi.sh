#!/bin/sh
if [ $1 ] ; then
    CC=$1
else
    CC="origin"
fi

./src/salsify/salsify-receiver 9090 1280 720 >log/recv_$1_$4.log 2>&1 &

nohup ffmpeg -re -i $3 -f v4l2 -s 1280x720 /dev/video0 >log/ffmpeg.log 2>&1 &
sleep 2s

mm-delay 10 mm-link $2 $2 --meter-uplink --meter-uplink-delay -- sh -c "./src/salsify/salsify-sender -d /dev/video0 -r ${CC} -p YU12 100.64.0.1 9090 1337 1280 720" >log/sender_$1_$4.log 2>&1 &
