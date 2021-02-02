./src/salsify/salsify-receiver  9090 1280 720 >log/recv.log 2>&1 &

nohup ffmpeg -re -i $2 -f v4l2 -s 1280x720 /dev/video0 >log/ffmpeg.log 2>&1 &

sleep 2s

./src/salsify/salsify-sender -d /dev/video0 -p YU12 -r $1 127.0.0.1 9090 1337 1280 720 > log/sender.log 2>&1 &
