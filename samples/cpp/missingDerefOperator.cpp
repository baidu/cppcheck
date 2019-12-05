void demo()
{
    *pattern_dict = lex_create_bin(dictpath);
    if (pattern_dict == NULL) {
        ul_writelog(UL_LOG_FATAL, "Load dict error!\n");
        return -1;
    }
}