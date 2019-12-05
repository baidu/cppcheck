void demo()
{
    struct STemp
    {
        int a = 0;
        int b = 0;
    };

    STemp sTemp = 0;
    // sTemp.a没有初始化
    int c = sTemp.a;
    c++;
}