STNullPointer* demo_func()
{
    if (rand() > 1024)
    {
        STNullPointer* np_st = new STNullPointer;
        return np_st;
    }
    return nullptr;
}

int demo()
{
    STNullPointer* np_st = demo_func();
    // ReturnNULLFunction可能返回空，此处没有判空直接使用
    int n_result = np_st->m_node;  // error
    ClearMemory(np_st);
    return n_result;
}