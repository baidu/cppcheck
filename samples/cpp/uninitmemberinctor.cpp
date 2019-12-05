class Demo{
public:
    demo();
private:
    int c;
    int b;
};

Demo::demo()
{
    //构造函数中c没有初始化就直接使用
    std::cout << c;
}