# chatserver
基于muduo库实现的集群聊天服务器 

项目是跟着腾讯课堂施磊老师做的，并在此基础上新增了：

1、加好友的验证功能：用了比较原始的方法————新创建了一个FriendVerify数据表，用来存储好友的验证信息，同时提供verify方法用于同意好友验证
2、业务逻辑补充：重复添加好友、添加未注册好友的消息反馈
             与非好友id直接聊天的消息反馈
             加入一个不存在的群聊和重复加群的消息反馈
             添加好友时使用户id和friendid互为好友

编译方式：
cd build
rm -rf *
cmake ..
make

需要安装nginx、redis
