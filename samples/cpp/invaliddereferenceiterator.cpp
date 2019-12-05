void demo(std::vector<int>& ivec)
{
    std::vector<int>::iterator iter;
    std::vector<int>::iterator iter2;
    for (iter = ivec.begin(); iter != ivec.end(); iter++) {
    }
    // iter此时可能指向了end()，解引用出错
    cout << *iter;
}