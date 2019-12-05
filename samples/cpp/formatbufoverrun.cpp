void demo() {
    char sz_str_time[5];
    snprintf(sz_str_time);//szStrTime可能溢出
    snprintf_s(sz_str_time, 4+1, "%s", szBuff);//szStrTime可能溢出
    sprintf_s(sz_str_time, 4+1, "%s", szBuff);//szStrTime可能溢出
}

int main() {
    demo();
}