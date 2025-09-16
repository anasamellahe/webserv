#pragma once

#include "ResponseBase.hpp"

class ResponsePost : public ResponseBase {
public:
    ResponsePost(Request& request);
    virtual ~ResponsePost();
protected:
    virtual void handle();
};
