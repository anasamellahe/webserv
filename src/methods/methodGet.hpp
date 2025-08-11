#pragma once
#include "../HTTP/Request.hpp"

class GETMETHOD
{
    private:
        Request &request;
        std::string Response;
        // what i need to start create the Get method 
        // i need to get the path of the server or in other word the side that the server handle and get data from
        // a Server config holder that hold all the data related to the server 
        // a request reference that hold the request parsed  data 
    public:
        GETMETHOD(Request& request);
        std::string GetDataFromServer();
        bool CheckIfServerSupportMethod();
        bool isDataOnServer();
        ~GETMETHOD();
        // a constructor  to set up all the data that accept a request object and a  

};