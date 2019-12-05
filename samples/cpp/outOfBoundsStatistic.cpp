void demo()
{
    int *p = NULL;
    int myarray[6];
    myarray[0] = 0;
    myarray[1] = 1;
    myarray[2] = 2;
    myarray[3] = 3;
    myarray[4] = 4;
    myarray[5] = 5;
    int j = 0;
    for (int i = 0; i < 6; i++) {
        j += 2;
        *p = myarray[i];
        *p = myarray[j];
    }
}