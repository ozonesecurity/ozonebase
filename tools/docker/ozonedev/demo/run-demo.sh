#/bin/bash
export DBG_PRINT=0
export DBG_LEVEL=0
export DBG_LOG=
/usr/bin/supervisord -c supervisord.conf
