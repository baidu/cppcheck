//Check if an exception could cause a leak in an argument constructed with
// shared_ptr/unique_ptr. For example, in the following code, it is possible
// that if g() throws an exception, the memory allocated by "new int(42)"
// could be leaked. See stackoverflow.com/questions/19034538/
// why-is-there-memory-leak-while-using-shared-ptr-as-a-function-parameter
void demo() {
    f(shared_ptr<int>(new int(42)), g());
}