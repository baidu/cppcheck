C* demo()
{   
    C* obj;
    return obj;
}
void demo_test()
{
    if(demo() != NULL) // obj判空
    {
        demo()->dosth();
    }
    // obj解引用，此时对于obj的判空保护已经失效
    demo()->dosth2();
}