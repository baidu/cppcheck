int demo(int iMax)
{
    int i_count = 0;
    while (i_count < iMax)
    {
        //没有任何条件的break一定会执行，会导致循环异常中断
        break;
        i_count++;
    }
    return i_count;
}