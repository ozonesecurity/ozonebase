/** @addtogroup Protocols */
/*@{*/


#ifndef OZ_RTMP_H
#define OZ_RTMP_H

#include "ozRtmpController.h"
#include "../libgen/libgenException.h"

class RtmpException : public Exception
{
public:
    RtmpException( const std::string &message ) : Exception( message )
    {
    }
};

#endif // OZ_RTMP_H


/*@}*/
