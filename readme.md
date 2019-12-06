# BCA-Cpp

BCA-Cpp是一个针对C/C++代码的分析工具,内核基于开源工具cppcheck,并集成了其他工具优秀的规则，结合百度大量的研发实践不断扩展规则并进行误报优化。
它能够发现代码中存在的BUG，并且关注于发现未定义的行为和一些危险的代码构造。

#### 特点
- 通过特别的分析检测代码中各种错误。
- 命令行界面和图形用户界面都可用。
- 非常注重检测未定义的行为。
- 较Cppcheck扩展了内存泄漏场景的检测。
- 基于百度大量研发实践进行误报优化。

## 快速开始

#### 准备工作
下载项目并进入根目录

```
cd bcacpp
```

#### 如何构建
拷贝打包后的压缩文件target/bcacpp*.zip到指定目录，解压即可。
进入根目录，运行

```
./install.sh
```

```
make
```
参考：[cppcheck参考手册](http://cppcheck.sourceforge.net/manual.pdf)
#### 如何运行
BCA-Cpp基本保留了cppcheck的执行方式，在根目录下运行

```
cppcheck path
```
参数：
- -i 可以指定要忽略的文件/路径 
`cppcheck -i src/a src` 检测src目录下所有.cpp&.c文件，忽略src/a中的文件。
`cppcheck -i src/b -i src/c src` 忽略多个文件
-  \- -project=<file> 可以对单个项目文件（.vcxproj）或整个解决方案运行Cppcheck
`cppcheck --project=foobar.sln` 在整个Visual Studio解决方案上运行Cppcheck
`cppcheck --project=foobar.vcxproj` 在Visual Studio项目上运行Cppcheck
- -f, --force 强制检查文件中的所有配置。如果与“--max configs=”一起使用，则最后一个选项是有效的
- --template='\<text>' 使用模板格式化输出
`cppcheck --template=vs path`   获取与Visual Studio兼容的输出
## 测试

src/test目录下包含测试源码目录。运行`cppcheck bcacpp/test/testsymboldatabase.cpp`，如果正常会得到类似下面的结果：

> bcacpp/test/testsymboldatabase.cpp(4393): error: Comparing [tok] to
> null at line 4390 implies that [tok ] might be null.Dereferencing null pointer [tok].
> 
> bcacpp/test/testsymboldatabase.cpp(5266): error: Comparing [autotok]
> to null at line 5260 implies that [autotok ] might be null.Dereferencing null pointer

## 自定义check

#### step1
Bca-Cpp会在读取源代码并在使用规则之前对其进行处理。
旨在查找错误和危险代码。文体信息（如缩进、注释等）在早期被过滤掉。当你写规则的时候，你不需要担心这样的文体信息。
代码中的每个标记之间总是有一个空格。例如，原始代码“1+f()”被处理为“1 + f（ ）”。
使用的数据表示是专门为静态分析而设计的。在工具中，数据在许多方面被简化了。
**预处理**
原始代码 `#define SIZE 123 char a[SIZE];`
BCA-Cpp的数据的是  `char a [ 123 ] ;`

**类型定义**
原始代码 `typedef char s8; s8 x;`
BCA-Cpp的数据是  `; char x ;`

**计算**
原始代码 `int a[10 + 4];`
BCA-Cpp的数据是  `int a [ 14 ] ;`
**变量**

- 变量声明，变量声明被简化。一次只能声明一个变量。初始化也被分解成一个单独的语句。
原始代码 `int *a=0, b=2;`
BCA-Cpp的数据是  `int * a ; a = 0 ; int b ; b = 2 ;`
- 已知变量的值

原始代码
```cpp
void f() {    
	int x = 0;    
	x++;    
	array[x + 2] = 0; 
}
```
BCA-Cpp的数据是

```cpp
void f ( ) { 
	; ;  
	;
	array [ 3 ] = 0 ; 
}
```

> 变量x将被删除，因为它在简化后未被使用。因此，这是多余的。“已知值”不必是数字。变量别名、指针别名、字符串等也应该处理。

例

```cpp
void f() {    
	char *a = strdup("hello");    
	char *b = a;    
	free(b); 
}
```
BCA-Cpp的数据是

```cpp
void f ( ) 
{ 
	char * a@1 ; a@1 = strdup ( "hello" ) ; 
	; ; 
	free ( a@1 ) ; 
}
```
**参考** [cppcheck数据表示](https://sourceforge.net/projects/cppcheck/files/Articles/writing-rules-2.pdf/download)
#### step2 
[创建规则表达式](https://sourceforge.net/projects/cppcheck/files/Articles/writing-rules-1.pdf/download)




## 如何贡献

1. 贡献代码类型
  -- 项目优化与bug修复
  -- 新增扫描规则
2. 贡献代码步骤
-- 添加百度hi讨论组，与项目Owner讨论此需求的必要性
-- 确认需求后，需拉分支进行本地开发，并进行线下测试
-- 测试完成后，提交代码到icode上
-- 项目Owner评审通过并合入
3. 质量要求
-- 项目优化与bug修复必须通过单元测试
-- 通过项目Owner的人工review

## 维护者

 Owners：bugbye@baidu.com

 


