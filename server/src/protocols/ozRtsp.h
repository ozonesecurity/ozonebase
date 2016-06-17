/** @addtogroup Protocols */
/*@{*/


#ifndef ZM_RTSP_H
#define ZM_RTSP_H

#include "ozRtspController.h"
#include "../libgen/libgenException.h"

class RtspException : public Exception
{
public:
    RtspException( const std::string &message ) : Exception( message )
    {
    }
};

#endif // ZM_RTSP_H


/*@}*/
