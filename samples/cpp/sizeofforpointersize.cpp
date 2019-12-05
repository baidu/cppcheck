void demo(CPointer* pArg) 
{
    if (pArg != nullptr)
    {
        char *sz_buff = pArg->sz_buff;
        if (sz_buff != nullptr)
        {
            // 这里sizeof指针是错误的
            memset(sz_buff, 0, sizeof(sz_buff));
        }
    }
}