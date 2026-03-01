import socket
import struct
import json

class RpcClient:
    def __init__(self, ip, port):
        self.ip = ip
        self.port = port
        # 定义包头格式：'>I' 表示 大端序(Big-Endian) 无符号整型(4字节)
        # 如果服务端是小端序(Little-Endian)，请改为 '<I'
        self.length_format = '>I' 
        self.length_size = struct.calcsize(self.length_format)

    def _pack_string(self, text):
        """将字符串转换为 [长度(4B)] + [内容] 的字节流"""
        if isinstance(text, str):
            data = text.encode('utf-8')
        else:
            data = text # 假设已经是 bytes
            
        length = len(data)
        # pack: 将整数打包成字节
        return struct.pack(self.length_format, length) + data

    def CallFunc(self, svc, mth, params_dict):
        """
        svc: 服务名 (str)
        mth: 方法名 (str)
        params_dict: 参数字典，将被转为 JSON
        """
        # 1. 准备 JSON 数据
        # ensure_ascii=False 支持中文，separators 去除多余空格以减小包体
        json_str = json.dumps(params_dict, ensure_ascii=False, separators=(',', ':'))
        
        # 2. 构建二进制包
        # 部分 A: [服务名长度][服务名]
        part_svc = self._pack_string(svc)
        
        # 部分 B: [方法名长度][方法名]
        part_mth = self._pack_string(mth)
        
        # 部分 C: [JSON长度][JSON数据]
        json_str = '{"name":"zhangsan","pwd":"123456"}'
        part_json = self._pack_string(json_str)
        
        # 拼接最终数据包
        request_data = part_svc + part_mth  + part_json
        
        # 3. 发送请求
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            # 设置超时，防止服务端无响应导致永久阻塞
            client.settimeout(5.0) 
            client.connect((self.ip, self.port))
            
            # 发送所有数据
            client.sendall(request_data)
            
            # --- 接收响应 (关键：如何知道接收完了？) ---
            # 假设响应也遵循同样的 [长度][数据] 格式，或者服务端会关闭连接
            # 这里演示一种通用的接收方式：先读4字节获取长度，再读剩余内容
            response_data = self._recv_all(client)
            
            if response_data:
                return response_data.decode('utf-8')
            else:
                return None
                
        except socket.timeout:
            print("Error: 请求超时")
            raise
        except Exception as e:
            print(f"Error: {e}")
            raise
        finally:
            client.close()

    def _recv_all(self, sock):
        """
        辅助函数：处理 TCP 粘包/拆包，确保读完完整的一个消息。
        这里假设响应格式也是：[4字节长度][实际数据]
        如果服务端只是发完就断开连接，逻辑需调整为 recv 直到 close。
        """
        # 1. 读取长度头 (4字节)
        raw_len = sock.recv(self.length_size)
        if not raw_len or len(raw_len) < self.length_size:
            return b""
        
        # 解包长度
        msg_len = struct.unpack(self.length_format, raw_len)[0]
        
        # 2. 循环读取直到凑齐 msg_len 字节
        chunks = []
        bytes_recd = 0
        while bytes_recd < msg_len:
            # 一次读一部分，避免阻塞或读太少
            chunk = sock.recv(min(msg_len - bytes_recd, 4096))
            if not chunk:
                raise ConnectionError("连接意外中断，数据接收不完整")
            chunks.append(chunk)
            bytes_recd += len(chunk)
            
        return b"".join(chunks)

# === 使用示例 ===
if __name__ == "__main__":
    client = RpcClient("127.0.0.1", 8080)
    
    # 构造参数
    params = {
        "name": "zhangsan",
        "pwd": "123456",
    }
    
    try:
        # 调用远程方法
        result = client.CallFunc("UserService", "Login", params)
        print(f"收到响应: {result}")
    except Exception as e:
        print(f"调用失败: {e}")