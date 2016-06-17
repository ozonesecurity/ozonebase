/** @addtogroup Protocols */
/*@{*/


#ifndef ZM_RTMP_H
#define ZM_RTMP_H

#include "zmRtmpController.h"
#include "../libgen/libgenException.h"

class RtmpException : public Exception
{
public:
    RtmpException( const std::string &message ) : Exception( message )
    {
    }
};

#endif // ZM_RTMP_H


/*@}*/
