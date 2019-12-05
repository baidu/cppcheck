bool rand()
{
    return rand() > 10;
}

void demo()
{
    // Rand()返回的是bool变量
    if (rand() > 3)
        DoSomething();
}