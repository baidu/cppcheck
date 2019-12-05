void demo1(struct RD_PACK_DETAIL * pack, char * buf) {
    int len = strlen(buf) + 1;
    //
    len = (len > RD_PVDE_LEN_MAX) ? RD_PVDE_LEN_MAX : len;
    memcpy(pack->pvde, buf, len);
    pack->pvde[len] = '\0';
    //
    return strlen(pack->pvde);
}

void demo(int b)
{
    int max = 5;
    int a[10];
    if (b > 0)
    {
        max = 10;
    }

    // max在b>0的时候等于10，造成数组越界
    a[max] = max;
}