void demo()
{
    //if(f != NULL) // obj判空
    //{
    //    f->dosth();
    //}
    // obj解引用，此时对于obj的判空保护已经失�?
    //f->dosth2();
    if (fp[i] != NULL) {
        //fclose(fp[i]);
    }
    fclose(fp[i]);
}
/*
void ClusterManager::CloseSession ( const std::string &amp; cluster , const std::string &amp; user , const std::string &amp; path , long session_id )
{    ctx->addr = INVALID_ESPADDRESS;
     ctx->lparam = 0;
     ctx->client = client;
     ctx->cluster = cluster;
     req->user_arg = ctx;
 
     m_close_count++;
     CC_NOTICE(&quot;begin close session:0x%lx, cluster:%s, user:%s, path:%s, close count:%u&quot;,
               session_id, ctx->cluster.c_str(), req->user_name.c_str(),
              req->path.c_str(), m_close_count);
    ctx->client->Close(req, req->session_id, false);
 }

 void ClusterManager::Shutdown(AgentShutdownCtx* ctx) {
    int stop_thread_id = sys_gettid();
    int thread_id = g_pExecMan->GetWorkerThreadId(m_nHostId);
    if (stop_thread_id == thread_id) {

    if (stop_thread_id == thread_id) {
         CC_NOTICE(&quot;stop thread:%d is same with cluster manager thread:%d, directly stop&quot;,
                   stop_thread_id, thread_id);
         ctx->cond->Signal();
         delete ctx;
}*/