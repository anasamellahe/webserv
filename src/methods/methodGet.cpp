#include "methodGet.hpp"

GETMETHOD::GETMETHOD(Request& request): request(request)
{
    Response = "";
}

std::string GETMETHOD::GetDataFromServer()
{
    //const Config::ServerConfig* serverConfig = request.getCurrentServer();
    

}

bool GETMETHOD::CheckIfServerSupportMethod()
{
    const Config::ServerConfig* serverConfig = request.getCurrentServer();
     if (serverConfig) {
    
        
        // Check if GET is supported by looking at routes
        for (size_t i = 0; i < serverConfig->routes.size(); i++) {
            const Config::RouteConfig& route = serverConfig->routes[i];
            if (route.path == request.getPath()) {
                // Check if GET is in route.accepted_methods .... 
                
                
               
            }
        }
    }
    return false;
}
bool GETMETHOD::isDataOnServer()
{

}

GETMETHOD::~GETMETHOD()
{

}