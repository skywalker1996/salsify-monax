import torch
import piq
from skimage.io import imread
import time 
import cv2
import pyzbar.pyzbar as pyzbar
import os
import numpy as np

raw_videoPath = '../videos/p01_720p.mp4'

IMAGE_DIR = "../images"

@torch.no_grad()
def main():
    
    recv_frame_pointer = 0  
    raw_frame_update = True
    recv_frame_update = True

    psnr_record = []
    ssim_record = []

    video_raw = cv2.VideoCapture(raw_videoPath)
    recv_images = os.listdir(IMAGE_DIR)
    recv_images = sorted(recv_images, key = lambda img: int(img.split('.')[0]))
    
    _, raw_frame = video_raw.read()
    raw_frame = raw_frame[...,::-1]
    
    recv_frame = cv2.imread(os.path.join(IMAGE_DIR, recv_images[recv_frame_pointer]))
    recv_frame = recv_frame[...,::-1]

    count = 0
    
    while(True):
        count+=1
        height = raw_frame.shape[0]
        width = raw_frame.shape[1]

        if(raw_frame is None):
            break
        
        if(raw_frame_update):
            gray_raw = cv2.cvtColor(raw_frame, cv2.COLOR_RGB2GRAY)
            raw_code = gray_raw[0:int(height/2), 0:int(width/2)]
            try:
                raw_frame_id = int(pyzbar.decode(raw_code)[0].data.decode('utf-8'))
            except:
                _, raw_frame = video_raw.read()
                raw_frame = raw_frame[...,::-1]
                continue
            raw_frame_update = False
    
        
        if(recv_frame_update):
            gray_recv = cv2.cvtColor(recv_frame, cv2.COLOR_RGB2GRAY) 
            recv_code = gray_recv[0:int(height/2), 0:int(width/2)] 
            try:
                recv_frame_id = int(pyzbar.decode(recv_code)[0].data.decode('utf-8'))
            except:
                recv_frame_pointer+=1
                recv_frame = cv2.imread(os.path.join(IMAGE_DIR, recv_images[recv_frame_pointer]))
                recv_frame = recv_frame[...,::-1]
                continue

            recv_frame_update = False
        
        if(raw_frame_id==recv_frame_id):
            # frame id matches, compute PSNR and SSIM score
            raw_tensor = torch.tensor(np.ascontiguousarray(raw_frame)).permute(2,0,1) / 255.
            recv_rensor = torch.tensor(np.ascontiguousarray(recv_frame)).permute(2,0,1) / 255.
            psnr_index = piq.psnr(raw_tensor, recv_rensor, data_range=1., reduction='none')
            ssim_index = piq.ssim(raw_tensor, recv_rensor, data_range=1.)
            
            _, raw_frame = video_raw.read()
            raw_frame = raw_frame[...,::-1]
            
            recv_frame_pointer+=1
            if(recv_frame_pointer==len(recv_images)):
                break
            recv_frame = cv2.imread(os.path.join(IMAGE_DIR, recv_images[recv_frame_pointer]))
            recv_frame = recv_frame[...,::-1]

            raw_frame_update = True
            recv_frame_update = True

            psnr_record.append(psnr_index)
            ssim_record.append(ssim_index)
            if(count%100==0):
                print(f"Frame id = {raw_frame_id} || PSNR = {psnr_index} and SSIM = {ssim_index}")

        elif(raw_frame_id < recv_frame_id):
            _, raw_frame = video_raw.read()
            raw_frame = raw_frame[...,::-1]
            raw_frame_update = True
  
        elif(raw_frame_id > recv_frame_id):
            
            recv_frame_pointer+=1
            if(recv_frame_pointer==len(recv_images)):
                break
            recv_frame = cv2.imread(os.path.join(IMAGE_DIR, recv_images[recv_frame_pointer]))
            recv_frame = recv_frame[...,::-1]
            recv_frame_update = True
        
    psnr_avg = np.mean(psnr_record)
    ssim_avg = np.mean(ssim_record)
    print(f"psnr_avg = {psnr_avg} and ssim_avg = {ssim_avg}")      


if __name__ == '__main__':
    main()