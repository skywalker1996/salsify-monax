## Salsify环境搭建配置

#### git仓库

```shell

[1] git clone https://github.com/excamera/alfalfa.git   

[2] git clone https://github.com/witaly-iwanow/alfalfa.git

```

[1]是作者提供的源码，只支持g++<=7    
[2]是修改过的源码，支持更新的g++版本



#### 依赖环境
* `g++` >= 5.0 :  C++编译环境
* `yasm` :  英特尔x86架构下的一个汇编器和反汇编器
* `libxinerama-dev` 
* `libxcursor-dev`
* `libglu1-mesa-dev`
* `libboost-all-dev`
* `libx264-dev` :  H.264/AVC编解码器
* `libxrandr-dev`
* `libxi-dev`
* `libglew-dev`
* `libglfw3-dev`


```shell

sudo apt-get update

sudo apt-get install g++ yasm libxinerama-dev libxcursor-dev libglu1-mesa-dev libboost-all-dev libx264-dev libxrandr-dev libxi-dev libglew-dev libglfw3-dev

```

#### 编译过程

进入clone后的源代码目录，执行以下命令进行编译过程：
```shell

./autogen.sh

./configure

make -j$(nproc)  # 多核make编译

sudo make install

```

我新加了一个compile.sh脚本，直接运行`sh compile.sh`即可

#### 工具安装及使用流程

* `ffmpeg` :  是一套可以用来记录、转换数字音频、视频，并能将其转化为流的开源计算机程序，包含了非常先进的音频/视频编解码库libavcodec。
* 
  ```shell

  sudo apt-get install ffmpeg

  ```



* `Guvcview` :  是一个[网络摄像头](https://en.wikipedia.org/wiki/Webcam)应用程序，即用于[Linux](https://en.wikipedia.org/wiki/Linux)桌面的处理[UVC](https://en.wikipedia.org/wiki/USB_video_device_class)流的软件，使用两个窗口的界面，一个窗口显示要记录的摄像机图像，另一个窗口显示设置和控件以及菜单。

  ```shell

  sudo apt-get install guvcview

  ```

* `mahimahi` :  A set of lightweight tools for browser developers, website authors, and network protocol designers that provides accurate measurements when recording and replaying HTTP content over emulated network conditions.
* 
  website:  http://mahimahi.mit.edu/


* `v4l2loopback-dkms` :  create a fake webcam.

  ```shell

  sudo apt-get install v4l2loopback-dkms

  ll /dev/video* # look for all camera module

  v4l2-ctl -d /dev/video0 --all # look fot the all information of video module

  modprobe v4l2loopback # start the fake camera module

  ```

其中在ubuntu18.04上安装v4l2loopback-dkms会出现 Bad Address的问题。

需要使用源代码进行编译安装（但是还是可能出问题，最好用ubuntu20.04）：    


1. Unload previous v4l2loopback：`sudo modprobe -r v4l2loopback `

2. `git clone https://github.com/umlaeute/v4l2loopback/`  

3. compile and install
```
make && sudo make install 
sudo depmod -a 
```
4. Load the videodev drivers 
```
sudo modprobe videodev  
sudo insmod ./v4l2loopback.ko devices=1 video_nr=2 exclusive_caps=1   
//Change video_nr based on how many cams you already got. Zero indexed
ls -al /dev/video* 
```
Use /dev/video[video_nr] with ffmpeg
```
sudo ffmpeg -f x11grab -r 60 -s 1920x1080 -i :0.0+1920,0 -vcodec rawvideo -pix_fmt yuv420p -threads 0 -f v4l2 -vf 'hflip,scale=640:360' /dev/video2
```
https://unix.stackexchange.com/questions/528400/how-can-i-stream-my-desktop-screen-to-dev-video1-as-a-fake-webcam-on-linux





#### 使用流程

视频文件放在./videos中，带宽trace文件放在./traces中

1. 不使用mahimahi

* 原始算法 `sh run.sh origin video_path`   
* monax算法 `sh run.sh monax  video_path`   

video_path 改成自己的视频文件地址，例如 ./videos/test.mp4


2. 使用mahimahi

* 原始算法 sh run_mahimahi.sh origin  trace_path  video_path
* monax算法 sh runmahimahi.sh monax  trace_path  video_path

video_path 改成自己的视频文件地址，例如 ./videos/test.mp4   
trace_path 改成自己的trace文件，例如./traces/Belgium_4GLTE/report_bus_0001.log


#### 添加算法

在net目录中添加自己的cc和hh文件，然后编辑Makefile.am，把两个文件添加进去，然后在salsify-sender中include并使用，重新编译即可。



### 附录：opencv库安装

#### 1. 下载openc源码
`git clone https://github.com/opencv/opencv.git`

安装依赖: `sudo apt-get install libopencv-dev`

#### 2. 编译opencv
```
mkdir release
cd release
cmake -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr/local ..
make -j8
sudo make install

编译完成之后添加入系统的共享链接库
sudo gedit /etc/ld.so.conf.d/opencv.conf
末尾添加一行 /usr/local/lib
sudo ldconfig       # 更新一下系统的共享链接库

# 在VS Code 中添加 includePath
ctrl + shift + p 键入 Edit Configurations, 会自动打开 c_cpp_properties.json
在includePath项里面添加：/home/librah/Desktop/workspace/opencv/**

```

#### 3. 使用opencv
```
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/imgproc/types_c.h>
#include <opencv2/highgui/highgui_c.h> 
```


#### Makefile.am 引入opencv库

```
AM_CPPFLAGS += `pkg-config opencv4 --cflags`
LIBS = `pkg-config opencv4 --libs` -ljpeg
```


