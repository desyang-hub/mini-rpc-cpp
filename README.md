## Mini-rpc

### 组件

- rpc_provider // rpc 服务器，服务提供方
- rpc_channel /// rpc 交流通道，通过它来链接到rpc_provider
- serializer // 序列化器，用于对数据进行编解码

业务层
- IService

- ServiceImpl // 业务实现
- ServiceProxy // 业务代理


std::string 作为一个局部变量时，不能够将.data() 或.c_str() 返回，局部变量销毁后，这个数据将会被释放