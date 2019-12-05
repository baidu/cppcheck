void demo(int condition) {
    condition = 1;
    char *cp = (char *)malloc(sizeof(char)*size);
    if (condition) {
        free(cp);
    }
    *cp  = 'a';
    if (*cp == 'a') {
        return;
    }
    free(cp); // Noncompliant 
}