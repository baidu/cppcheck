void demo()
{
    if (f[i] != NULL) // obj判空
    {
        f[i]->dosth();
    }
    // obj解引用，此时对于obj的判空保护已经失效
    f[i]->dosth2();
}