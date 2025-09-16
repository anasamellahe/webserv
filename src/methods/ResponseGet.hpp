#pragma once

#include "ResponseBase.hpp"

class ResponseGet : public ResponseBase {
public:
    ResponseGet(Request& request);
    virtual ~ResponseGet();
protected:
    virtual void handle();
private:
    std::string contentTypeFromPath(const std::string &path);
};
