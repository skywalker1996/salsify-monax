ps -e|grep ffmpeg|awk '{print $1}'|xargs kill 9
ps -e|grep salsify|awk '{print $1}'|xargs kill 9
ps -e|grep mm-link|awk '{print $1}'|xargs kill 9
ps -e|grep mm-delay|awk '{print $1}'|xargs kill 9
