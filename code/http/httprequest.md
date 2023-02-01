1.urlencoded 格式，又叫 form 格式、x-www-form-urlencoded 格式
它是一种表单格式
组成格式
键值对组成
键和值之间用 = ：name=poloyy
多个键值对之间用 & ：name=poloyy&age=19
若请求query中包含中文，中文会被编码为 %+16进制+16进制形式
提交表单时请求时Content-Type：application/x-www-form-urlencoded的情况下，URL请求查询字符串中出现空格时，需替换为+。

application/x-www-form-urlencoded：默认的编码方式。但是在用文本的传输和MP3等大型文件的时候，使用这种编码就显得 效率低下。
multipart/form-data：指定传输数据为二进制类型，比如图片、mp3、文件。
text/plain：纯文体的传输。空格转换为 “+” 加号，但不对特殊字符编码。
2.multipart/form-data是基于post方法来传递数据的，并且其请求内容格式为Content-Type: multipart/form-data,用来指定请求内容的数据编码格式。另外，该格式会生成一个boundary字符串来分割请求头与请求体的，具体的是以一个boundary=${boundary}来进行分割
...
Content-Type: multipart/form-data; boundary=${boundary}

--${boundary}
...
...

--${boundary}--
从上面的 multipart/form-data 格式发送的请求的样式来看，它包含了多个 Parts，每个 Part 都包含头信息部分，
Part 头信息中必须包含一个 Content-Disposition 头，其他的头信息则为可选项， 比如 Content-Type 等。
Content-Disposition 包含了 type 和 一个名字为 name 的 parameter，type 是 form-data，name 参数的值则为表单控件（也即 field）的名字，如果是文件，那么还有一个 filename 参数，值就是文件名。
Content-Disposition: form-data; name="user"; filename="hello.txt"

3.使用std::search直接搜索\r\n，避免了C语言里还需使用从状态机的情况

4.request判断错误码只能判断出BAD_REQUEST 400。