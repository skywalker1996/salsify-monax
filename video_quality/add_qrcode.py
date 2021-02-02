import cv2 
import qrcode
import numpy as np




def addQrcode(input_video, output_video):

	cap = cv2.VideoCapture(input_video)

	fps = int(round(cap.get(cv2.CAP_PROP_FPS)))
	width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
	height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
	size = (width, height)

	frame_id = 0
	fourcc = cv2.VideoWriter_fourcc(*'mp4v')

	vout = cv2.VideoWriter(output_video,fourcc,fps,size)

	while(True):
		ret, frame = cap.read()
		if(frame is None):
			vout.release()
			print('break!!')
			break

		qr=qrcode.QRCode(version = 2,error_correction = qrcode.constants.ERROR_CORRECT_L,box_size=6,border=1)
		qr.add_data(str(frame_id))
		qr.make(fit=True)
		qr_img = np.array(qr.make_image().convert("RGB"))
		qr_width = qr_img.shape[0]
		frame[0:qr_width,0:qr_width] = qr_img
		frame_id += 1

		vout.write(frame)
		cv2.imshow('video',frame)
		if cv2.waitKey(1) & 0xFF == ord('q'):
			vout.release()
			break


input_video = "../videos/p01.mp4"
output_video = "../videos/p01_qr.mp4"

addQrcode(input_video, output_video)
