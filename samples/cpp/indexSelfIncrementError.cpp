void demo(int b)
{
    int a[10];
    for (int i = 0; i < 10; i++) {
        a[i] = i;
        i++;
        b = a[i];
    }
}