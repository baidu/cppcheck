void demo() {
    char sz_str_time[5];
    snprintf(sz_str_time);//szStrTime�������
    snprintf_s(sz_str_time, 4+1, "%s", szBuff);//szStrTime�������
    sprintf_s(sz_str_time, 4+1, "%s", szBuff);//szStrTime�������
}

int main() {
    demo();
}