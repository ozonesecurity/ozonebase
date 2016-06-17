/** @addtogroup Protocols */
/*@{*/


#ifndef OZ_RTSP_H
#define OZ_RTSP_H

#include "ozRtspController.h"
#include "../libgen/libgenException.h"

class RtspException : public Exception
{
public:
    RtspException( const std::string &message ) : Exception( message )
    {
    }
};

#endif // OZ_RTSP_H


/*@}*/
