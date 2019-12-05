C* demo()
{
    C* obj;
    return obj;
}
void demo_test()
{
    demo()->dosth2();// obj解引用，此时对于obj的判空保护已经失效
    if (demo() != NULL) // obj判空
    {
        demo()->dosth();
    }
}