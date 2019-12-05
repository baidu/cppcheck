int demo(STNullPointer* npSt)
{
    if (npSt == nullptr)
    {
        // npSt为空时解引用
        int n_result = npSt->m_node;
        return n_result;
    }
    return 0;
}