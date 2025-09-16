#pragma once

#include "ResponseBase.hpp"

class ResponseDelete : public ResponseBase {
public:
    ResponseDelete(Request& request);
    virtual ~ResponseDelete();
protected:
    virtual void handle();
};
