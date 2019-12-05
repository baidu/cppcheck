class Line
{
private:
    int* _p;
public:
    Line(); //类Line的构造函数
    ~Line() {
        delete _p;
    }; //类Line的构造函数

    int* get_len(){
        _p = new int[5];
        _p = new int[7];
        return _p;
    }
};

Line::Line() : _p(NULL)
{
}

void main()
{    
}