import requests
import json
import re
import sys
import ctypes 
import socket
import os
import time

def ttsTrans(con):
    #此函数用于文字转语音，具体参数请参考百度云开放平台 apike与 securtkey请自行到百度云开放平台申请
    #aue参数决定返回格式：3MP3   4pcm-rate16000   5 pcm-rate8000   6wav
    #vol 参数0-15音量大小  spd  语速 0-15 
    access_token_raw = requests.get('https://openapi.baidu.com/oauth/2.0/token?grant_type=client_credentials&client_id='+此处填写APIkey+'&client_secret='+此处填写securtkey).text
    access_token = json.loads(access_token_raw)['access_token']
    url = r'http://tsn.baidu.com/text2audio?tex='+ con + '&lan=zh&cuid=wqnmldb&ctp=1&tok='+access_token+'&spd=5'+'&vol=15'+'&per=111'+'&ctp=6'+'&aue=5'
    print(url)
    
    return requests.get(url).content
    """cc= requests.get(url)
    print(cc.headers)"""


    #写文件
    #f = open('a.mp3','wb')
    #f.write(the)
def getWeather():
    #爬取青岛天气
    x = requests.get('https://tianqiapi.com/api.php?style=tb&skin=pitaya').text
    pat = re.compile(r'"> <em>([\s\S]*?)</em> <em class="wTemp">([\s\S]*?)</em>')
    y = pat.findall(x)
    return "青岛"+str(y[0])+"  播放完毕，语音结束后将自动为您回到主菜单"
def oneWord():
    #获取一言
    word_json = json.loads(requests.get('https://v1.hitokoto.cn/').text)
    return word_json['hitokoto']
"""
def trans():
    Objdll = ctypes.windll.LoadLibrary("djcvt.dll")
    s_buf1 = '1.wav'
    s_buf2 = '2'
    Ptr1 = c_char_p()
    Ptr2 = c_char_p()
    Ptr1.value = s_buf1
    Ptr2.value = s_buf2
    r = Objdll.WavetoPcm(s_buf1,s_buf2)
    return r"""

def today():
    #百度新鲜事获取，未完成
    text = requests.get('https://www.baidu.com/s?wd=%E4%BB%8A%E6%97%A5%E6%96%B0%E9%B2%9C%E4%BA%8B').text 
    print(text)
    re_con = """<p class="op_exactqa_answer_prop c-gap-bottom-small c-gray" >([\s\S]*?)</p>
                        <div class="op_exactqa_answer_s">([\s\S]*?)
                            </div>
                                    <p class="c-gap-top-small"><span class="c-gap-right">([\s\S]*?)</span>"""
    patten = re.compile(re_con)
    y = patten.findall(text)
    print(len(y))

def getAddr():
    #获取本机内网ip地址
    pat = re.compile('IPv4 地址 . . . . . . . . . . . . : (172\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3})\(首选\)')
    result = re.findall(pat,os.popen('ipconfig/all').read())
    return result[0]


def main(ip):
    #此函数用于socket传输文件，无其他意义
    server = socket.socket(socket.AF_INET,socket.SOCK_STREAM,0)
    server.bind((ip,19730))
    server.listen(5)
    while True:

        client,addr = server.accept()
        print('enter')
        order = client.recv(1024)
        # 1--天气
        if order.decode('utf-8')=='1':
            print("进入1")
            wea = getWeather()
            print(wea)
            tts = ttsTrans(wea)
            wj = open("temp","wb")
            wj.write(tts)
            print(len(tts))
            #os.system("djcvtdll.exe")  #此处调用dll用于将语音转换到语音卡可支持的格式
            time.sleep(1)
            wj2 = open("temps","rb")
            aaa = wj2.read()
            print(len(aaa))
            client.send(aaa)
            wj2.close()
        else:
            #client.send(ttsTrans(order.decode('utf-8')))
            print(order.decode('utf-8'))
            



if __name__ == "__main__":
    
    #print(getWeather())
    #ttsTrans("asdkhjask")
    #print(os.popen('ipconfig/all').read())
    
    main(getAddr())

   # today()
