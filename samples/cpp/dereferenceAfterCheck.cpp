void demo(C* obj)
{
    // obj判空
    if (obj != NULL) {
        obj->dosth();
    }

    // obj解引用，此时对于obj的判空保护已经失效
    obj->dosth2();
}