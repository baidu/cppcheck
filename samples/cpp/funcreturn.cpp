int ret_func()
{
    if (rand() > 10)
    {
        return 10;
    }
    return 1;
}

void demo()
{
    //函数有返回值，却没有处理
    ret_func();
}