void demo(int n)
{
    //递归调用存在性能问题，可能导致栈溢出
    demo(n-1); 
}