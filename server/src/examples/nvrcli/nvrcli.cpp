#include "ozone.h"
#include <iostream>
#include <thread>
#include <string>
#include <cctype>
#include <algorithm>
#include <unordered_map>
#include <list>
using namespace std;


#define MAX_CAMS 10
list<NetworkAVInput *> cams;
list <MotionDetector *> motions;

Listener *listener;
HttpController* httpController;
Application app;
const char* const defRtspUrls[] = {
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

	if (cams.size() == MAX_CAMS)
	{
		cout << "Cannot add any more cams!\n\n";
		return;
	}

    string name;
    string source;

    cin.clear(); cin.sync();
    cout << "camera name (ENTER for default):";
    getline(cin,name);
    cout << "RTSP source (ENTER for default):";
    getline(cin,source);
	if (name.size()==0 )
	{
		string n = to_string(cams.size());
		name = "cam" + n;
	}
    if (source.size() == 0 )
    {
        source = defRtspUrls[cams.size() % MAX_CAMS];
    }
    
    
	NetworkAVInput *cam = new NetworkAVInput ( name, source );
	cams.push_back(cam);
	
	
    cout << "Adding:"<<cams.back()->name() << endl;
    cout << cams.back()->source() << endl;
    app.addThread( cams.back() );
    cams.back()->start();

    // motion detect for cam
    MotionDetector *motion = new MotionDetector( "modect-"+name );
	motions.push_back(motion);
    motion->registerProvider(*(cams.back()) );
    app.addThread( motions.back() );
    motions.back()->start();


    listener->removeController(httpController);
    httpController->addStream("live",*(cams.back()));
    httpController->addStream("debug",*(motions.back()));
    listener->addController(httpController);
}

void cmd_help()
{
    cout << endl << "Possible commands: add, delete, list, stop, exit" << endl;
}

void cmd_list()
{
	for (NetworkAVInput* c:cams)
	{
		cout << c->name() <<"-->"<<c->source() << endl;
	}
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
	cmd_map["list"] = &cmd_list;
    
    
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
    debugInitialise( "nvrcli", "", 0 );
    cout << " \n---------------------- NVRCLI ------------------\n"
             " Type help to get started\n"
             " ------------------------------------------------\n\n";

    Info( "Starting" );

    avInit();

    
    listener = new Listener;
    httpController = new HttpController( "watch", 9292 );
    listener->addController( httpController );
    app.addThread( listener );
    thread t1(cli,app);
    app.run();
    cout << "Never here";
}
