/** @addtogroup Protocols */
/*@{*/


#ifndef OZ_HTTP_H
#define OZ_HTTP_H

#include "ozHttpController.h"
#include "../libgen/libgenException.h"

class HttpException : public Exception
{
public:
    HttpException( const std::string &message ) : Exception( message )
    {
    }
};

#endif // OZ_HTTP_H


/*@}*/
