./src/salsify/salsify-receiver 9090 1280 720 >log/recv.info 2>&1 &

nohup ffmpeg -re -i ../demo.mp4 -f v4l2 -s 1280x720 /dev/video0 &

sleep 2s


./src/salsify/salsify-sender -d /dev/video0 -p YU12 127.0.0.1 9090 1337 > log/sender.info 2>&1 &
