#!/usr/bin/env Python
# coding=utf-8

from ctypes import *

import time
import numpy as np
import cv2
import struct

import datetime

from numba import jit

cam_dict={}

mmm = 0
@jit
def trans(data,size,height,width):
    bbb = string_at(data,size)
    nparr = np.fromstring(bbb, np.uint8)
    r = nparr.reshape(height,width, 3)
    return r
def str2char_p(str_v):
    pStr = c_char_p( )
    pStr.value = str_v
    return pStr
def callb_stream(data,size,cam_no,height,width):
    global mmm
    r = trans(data, size,height,width)
    counter = datetime.datetime.now().strftime('%Y%m%d_%H%M%S_%f')
    # print(1, counter)
    mmm+=1
    cv2.imwrite(r'E:\git_project\hik_client_dll_264\x64\Debug\bmp1/%d.jpg'%mmm,r)
    cv2.imshow(str(cam_no), r)
    cv2.waitKey(0)
    # k = cv2.waitKey(400) & 0xFF
    # if k == 13:
    #     cv2.waitKey()
    # elif k == ord('m'):
    #     cv2.waitKey()
    # elif k == 32:  # 空格
    #     cv2.waitKey()



if __name__ == '__main__':
    dll = CDLL(r"./hik_client.dll")
    CMPFUNC = CFUNCTYPE(c_void_p, c_void_p, c_int, c_int, c_int, c_int)
    m_callback = CMPFUNC(callb_stream)
    filename=b"D:\\project\\hik_client_dll\\x64\\Debug\\0_Data.h264"
    # filename = b"D:\\data\\test_video\\play_failed/201805032342.dat"

    # dll.play_file(str2char_p(filename), m_callback)

    dll.seek_certain_pic(str2char_p(filename),c_int(5),c_int(4), m_callback)
