aux_source_directory(. SRC_LIST)

add_executable(ChatClient ${SRC_LIST})

#两个线程，一个写一个读，写接受用户的输入，读接受数据
target_link_libraries(ChatClient pthread)