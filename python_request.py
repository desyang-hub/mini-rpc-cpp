import json

class Channel:
    def __init__(self, ip, port):
        self.ip = ip
        self.port = port

    def CallFunc(self, svc, mth, json_str):
        # 建立连接
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((self.ip, self.port))

        # 发起调用
        # 格式：[服务名长度(4B)][服务名][方法名长度(4B)][方法名][JSON 长度 (4B)][JSON 数据]


class UserServiceImpl:
    def Login(name :str, pwd :str):

        pass

import socket

def start_client():
    host = '127.0.0.1' # 服务器地址
    port = 8080        # 服务器端口

    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    try:
        client.connect((host, port))
        print(f"[+] 已连接到服务器 {host}:{port}")
        
        while True:
            msg = input("请输入消息 (输入 'quit' 退出): ")
            if msg.lower() == 'quit':
                break
            
            client.send(msg.encode('utf-8'))
            
            # 接收响应
            data = client.recv(1024)
            if not data:
                print("[-] 服务器断开连接")
                break
            print(f"[服务器回复]: {data.decode('utf-8')}")
            
    except Exception as e:
        print(f"[-] 发生错误: {e}")
    finally:
        client.close()
        print("[!] 连接关闭")

if __name__ == "__main__":
    start_client()