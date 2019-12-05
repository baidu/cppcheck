int* doubler(int a[], int size, bool flag) {
    int* temp = new int[size];
    /*if(flag) {
        for ( int i =0; i < size; i++) {
            temp[i] = 2*a[i];
        }
    }*/
    return temp;
}
void demo() {
    //int a[] = {1, 2, 3, 4, 5};
    int a[] = {};
    int* b = NULL;
    b = doubler(a, 5);
    delete[] b;
}