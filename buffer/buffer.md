## buffer

1. 本类中读写缓冲区是同一个，而不是两个，所以read_idx,write_idx含义有所不同。只要向缓冲区写数据writeidx就会增加，包括append
2. 扩容可以防止使用vsnprintf,va_list这种语法，因为不知道要输入的大小是多少，扩容可以实现。
3. 注意ensure函数的作用，判断能不能写进去，如果不够就扩容。所以一定要在ensure后再写入缓冲区。
