#include "nvrEventDetector.h"

#include <base/ozMotionFrame.h>
#include <libgen/libgenTime.h>
#include <iostream>

using namespace std;

// oZone expects a run method that does init for these classes. Write your own.
// don't call base run because it will run forever 
int EventDetector::run()
{
	if ( waitForProviders() )
	{
		while( !mStop )
		{
			mQueueMutex.lock();
			if ( !mFrameQueue.empty() )
			{
				for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
				{
					processFrame( *iter );
				}
				mFrameQueue.clear();
			}
			mQueueMutex.unlock();
			checkProviders();
			usleep( INTERFRAME_TIMEOUT );
		}
	}
	cleanup();
	return( 0 );

}

// do your own handling for processFrame and then call base processFrame
bool EventDetector::processFrame( FramePtr frame )
{
	EventRecorder::processFrame(frame);
	if (mState == ALARM)
	{
		cout << "Alarmed: "<<  mCam->name() << endl;
	}
}


