all: httpd client
LIBS = -lpthread #-lsocket
httpd: httpd.c
	gcc -g -W -Wall $(LIBS) -o $@ $<

client: simpleclient.c
	gcc -W -Wall -o $@ $<
clean:
	rm httpd
	


# 这是一个简单的`Makefile`示例，用于构建`httpd`和`client`两个可执行文件。以下是对该`Makefile`的解释：

# - `all`是一个伪目标，它是默认目标。在这个示例中，它指定了要构建的目标是`httpd`和`client`。
# - `LIBS`变量定义了要链接的库，这里使用了`-lpthread`。
# - `httpd`规则指定了生成`httpd`可执行文件的命令。它依赖于`httpd.c`文件，并使用`gcc`编译器进行编译和链接。
# - `client`规则指定了生成`client`可执行文件的命令。它依赖于`simpleclient.c`文件，并使用`gcc`编译器进行编译和链接。
# - `clean`规则指定了清理操作的命令，用于删除生成的可执行文件。

# 要使用该`Makefile`，将上述内容保存为名为`Makefile`的文件，并确保在同一目录下存在`httpd.c`和`simpleclient.c`文件。
# 然后，在命令行中运行`make`命令即可开始构建`httpd`和`client`可执行文件。运行`make clean`可以删除生成的可执行文件。	
