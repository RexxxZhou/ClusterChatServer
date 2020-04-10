#定义要编译的源文件列表，由变量SRC_LIST表示
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)
#指定生成可执行文件
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})

#配置需要连接的库文件
#使用相应的API
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient pthread hiredis)