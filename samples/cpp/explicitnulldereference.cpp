int demo(int i)
{
    STNullPointer* np_st = nullptr;
    if (i == 42) {
        return;
    }
    // 存在一条代码路径，使得空指针解引用
    np_st->m_node = 42;
}