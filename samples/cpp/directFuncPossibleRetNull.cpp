void demo()
{
    //GetIntance函数有可能返回空，没有做检查就直接解引用
    GetInstance()->SetID(nID);
}