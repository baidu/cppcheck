class Line
{
private:
    int* _p;
public:
    Line(); //��Line�Ĺ��캯��
    ~Line() {
        delete _p;
    }; //��Line�Ĺ��캯��

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