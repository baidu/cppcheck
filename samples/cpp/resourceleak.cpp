void demo()
{
    FILE *p_file = fopen("c:\\test.txt", "w+");
    if (p_file != nullptr)
    {
        // 没有close文件就退出了
        printf("forget to release pFile!");
        return;
    }
}