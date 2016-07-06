#include "ozone.h"
#include <iostream>
#include <thread>
#include <string>
#include <cctype>
#include <algorithm>
#include <unordered_map>
using namespace std;


#define MAX_CAMS 10
NetworkAVInput* cam[MAX_CAMS];
MotionDetector* motion[MAX_CAMS];
Listener *listener;
HttpController* httpController;
Application app;
int cam_ndx=-1;
const char* const defRtspUrls[] = {
   "rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov",
   "rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov",
   "rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov",
   "rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov",
   "rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov",
   "rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov",
   "rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov",
   "rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov",
   "rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov",
   "rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov"
};


// adds a new camera and motion detector
void cmd_add()
{
    string name;
    string source;

    cin.clear(); cin.sync();
    cout << "camera name:";
    getline(cin,name);
    cout << "RTSP source:";
    getline(cin,source);
    cam_ndx++;
	if (name.size()==0 )
	{
		string n = to_string(cam_ndx);
		name = "cam" + n;
	}
    if (source.size() == 0 )
    {
        source = defRtspUrls[cam_ndx];
    }
    
    cam[cam_ndx] = new NetworkAVInput ( name, source );
    cout << "Adding @index:"<<cam_ndx<<":"<<cam[cam_ndx]->name() << endl;
    cout << cam[cam_ndx]->source() << endl;
    app.addThread( cam[cam_ndx] );
    cam[cam_ndx]->start();

    // motion detect for cam
    motion[cam_ndx] = new MotionDetector( "modect-"+name );
    motion[cam_ndx]->registerProvider(*cam[cam_ndx] );
    app.addThread( motion[cam_ndx] );
    motion[cam_ndx]->start();

    // stop the listener, add new camera, restart
    //listener->stop(); // don't really need this, it seems
    //listener->join(); // don't really need this, it seems
	//cout << "listener killed\n";
    //delete listener;
   // listener->removeController(httpController);
    listener->removeController(httpController);
    httpController->addStream("live",*cam[cam_ndx]);
    httpController->addStream("debug",*motion[cam_ndx]);
    //listener = new Listener;
    listener->addController(httpController);
    //listener->start();
}

void cmd_help()
{
    cout << endl << "Possible commands: add, delete, list, stop, exit" << endl;
}

void cmd_unknown()
{
    cout << endl << "unknown command. try help"<< endl;
}
//  This thread will listen to commands from users

void cli(Application app)
{
    unordered_map<std::string, std::function<void()>> cmd_map;
    cmd_map["help"] = &cmd_help;
    cmd_map["add"] = &cmd_add;
    
    
    string command;
    for (;;) 
    { 
        cout << "?:";
        getline (cin,command);
        // really? no string lowercase?
        transform(command.begin(), command.end(), command.begin(), [](unsigned char c) { return tolower(c); });
        cout << "You entered: "<< command << endl;
        if (cmd_map.find(command) == cmd_map.end()) 
            cmd_unknown();
        else
            cmd_map[command]();
        cout <<endl<<endl;
    }
}

int main( int argc, const char *argv[] )
{
    debugInitialise( "nvrcli", "", 1 );
    cout << " \n---------------------- NVRCLI ------------------\n"
             " Type help to get started\n"
             " ------------------------------------------------\n\n";

    Info( "Starting" );

    avInit();

    //fixme: convert this to a list later
    for (int i=0; i<MAX_CAMS; i++) { cam[i] = NULL; motion[i] = NULL; }
    
    listener = new Listener;
    httpController = new HttpController( "watch", 9292 );
    listener->addController( httpController );
    app.addThread( listener );
    thread t1(cli,app);
    app.run();
    cout << "Never here";
}
