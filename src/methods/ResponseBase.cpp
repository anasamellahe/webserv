#include "ResponseBase.hpp"

ResponseBase::ResponseBase(Request& request)
    : request(request),
    statusCode(200),
    statusText("ok"),
    finalized(false)

{}

void ResponseBase::buildError(int code,  std::string text){
    if (!finalized){
        this->statusCode = code;
        this->statusText = text;
        this->body = buildDefaultBodyError(code);
        finalize();
    }
}

std::string ReadFromFile(const std::string &path)
{
    
}
std::string ResponseBase::buildDefaultBodyError(int code){

    std::string path;
    Config::ServerConfig::ErrorPagesIterator it;
    it =  request.serverConfig.error_pages.find(code);
    if (it != request.serverConfig.error_pages.end()){
        path =  it->second;
        try{
            this->body =  ReadFromFile(path);
        }
        catch(...)
        {

        }
    }
    else
        this->body =  GenerateDefaultError(code);

    // we need to add the config struct to the requests to have  a reference to the erro  pages 
}
const std::string & ResponseBase::generate(){
    if (!finalized){
        if (!isMethodAllowed())
             buildError(404, "Method Not Allowed");
        else{
            try{
                handle();
                finalize();
            }catch(...){
                buildError(500, "Internal Server Error");
            }
        }
    }
    return response;

}