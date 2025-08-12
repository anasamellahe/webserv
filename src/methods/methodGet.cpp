#include "methodGet.hpp"

GETMETHOD::GETMETHOD(Request& request): request(request)
{
    Response = "";
}

std::string GETMETHOD::GetDataFromServer()
{

}

bool GETMETHOD::CheckIfServerSupportMethod()
{
    //const Config::ServerConfig* serverConfig = request.getCurrentServer();
    

}

bool GETMETHOD::isDataOnServer()
{

}

GETMETHOD::~GETMETHOD()
{

}