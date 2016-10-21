#/bin/bash
#
# Debug config
#
export DBG_PRINT=0
export DBG_LEVEL=0
export DBG_LOG=
#
# Motion detection config
#
export OZ_OPT_MOTION_debug_streams=true
export OZ_OPT_MOTION_debug_images=false
export OZ_OPT_MOTION_debug_location="/tmp"
export OZ_OPT_MOTION_options_refBlend=7
export OZ_OPT_MOTION_options_varBlend=9
export OZ_OPT_MOTION_options_blendAlarmedImages=true
export OZ_OPT_MOTION_options_deltaCorrection=false
export OZ_OPT_MOTION_options_analysisScale=1
#export OZ_OPT_MOTION_zone_default_color=(Rgb)RGB_RED
export OZ_OPT_MOTION_zone_default_checkBlobs=true
export OZ_OPT_MOTION_zone_default_diffThres=1.0
# Unused - xport OZ_OPT_MOTION_zone_default_scoreThres=1.41
# Unused - export OZ_OPT_MOTION_zone_default_scoreBlend=64
export OZ_OPT_MOTION_zone_default_alarmPercent_min=0.5
export OZ_OPT_MOTION_zone_default_alarmPercent_max=0.0
export OZ_OPT_MOTION_zone_default_score_min=50
export OZ_OPT_MOTION_zone_default_score_max=0
#
# Start the processes
#
/usr/bin/supervisord -c supervisord.conf
