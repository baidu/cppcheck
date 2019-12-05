void do_something(int size) {
    char *cp = (char *)malloc(sizeof(char)*size);
    if (condition) {
        free(cp);
    }
    free(cp); // Noncompliant
}