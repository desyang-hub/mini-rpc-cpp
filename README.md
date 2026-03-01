## Mini-rpc

### 组件

- rpc_provider // rpc 服务器，服务提供方
- rpc_channel /// rpc 交流通道，通过它来链接到rpc_provider
- serializer // 序列化器，用于对数据进行编解码
- protocol // 用于数据的编码和解码

业务层
- IService

- UserServiceImpl // 业务实现
- UserServiceStub // 业务代理


std::string 作为一个局部变量时，不能够将.data() 或.c_str() 返回，局部变量销毁后，这个数据将会被释放

### 遇到的问题

- LT 触发模式下死循环问题：需要正确处理errno=EINTR和error=EWOULDBLOCK,直接break，不要continue

- 对于Vscode的CMake插件遇到一些问题，导致命令行可以编译，但是插件无法编译；可以直接使用指令以Debug模式编译程序，配置lunch.json文件进行程序调试