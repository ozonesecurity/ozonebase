/** @addtogroup Protocols */
/*@{*/


#ifndef ZM_HTTP_H
#define ZM_HTTP_H

#include "ozHttpController.h"
#include "../libgen/libgenException.h"

class HttpException : public Exception
{
public:
    HttpException( const std::string &message ) : Exception( message )
    {
    }
};

#endif // ZM_HTTP_H


/*@}*/
