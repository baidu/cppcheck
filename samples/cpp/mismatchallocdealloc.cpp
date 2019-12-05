void demo()
{
    short *p_short = new short;
    // new的对象应该使用delete来释放
    free(p_short);
    p_short = NULL;
}