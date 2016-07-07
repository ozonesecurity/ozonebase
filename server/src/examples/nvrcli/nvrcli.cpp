#include "ozone.h"
#include "ozEventDetector.h"
#include <iostream>
#include <thread>
#include <string>
#include <cctype>
#include <algorithm>
#include <unordered_map>
#include <list>
using namespace std;

struct nvrCameras
{
	NetworkAVInput *cam;
	MotionDetector *motion;	
	EventDetector *event;
	MovieFileOutput *movie;
};


#define MAX_CAMS 10
// TBD: convert these two lists into a single one
list <struct nvrCameras> nvrcams;
int camid=0; // id to suffix to cam-name. always increasing
Listener *listener;
HttpController* httpController;
Application app;

// default URLs to use if none specified
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


void eventCallback(string s)
{
	cout << "EVENT CALLBACK: " << s << endl;
}

// adds a new camera and motion detector
void cmd_add()
{

	if (nvrcams.size() == MAX_CAMS)
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
		string n = to_string(camid);
		camid++;
		name = "cam" + n;
	}
    if (source.size() == 0 )
    {
        source = defRtspUrls[nvrcams.size() % MAX_CAMS];
    }
    
    
	struct nvrCameras nvrcam;
	nvrcam.cam = new NetworkAVInput ( name, source );
    nvrcam.motion = new MotionDetector( "modect-"+name );
    nvrcam.motion->registerProvider(*(nvrcam.cam) );
	//nvrcam.event = new EventDetector( "event-"+name, eventCallback, "/tmp" );
	//nvrcam.event->registerProvider(*(nvrcam.motion));

	VideoParms* videoParms= new VideoParms( 640, 480 );
    AudioParms* audioParms = new AudioParms;
	nvrcam.movie = new MovieFileOutput(name, "/tmp/events", "mp4", 300, *videoParms, *audioParms);
	nvrcam.movie->registerProvider(*(nvrcam.motion));

	nvrcams.push_back(nvrcam); // add to list
	
    cout << "Added:"<<nvrcams.back().cam->name() << endl;
    cout << nvrcams.back().cam->source() << endl;

    nvrcams.back().cam->start();
    nvrcams.back().motion->start();
    nvrcams.back().movie->start();
    //nvrcams.back().event->start();


    listener->removeController(httpController);
    httpController->addStream("live",*(nvrcam.cam));
    httpController->addStream("debug",*(nvrcam.motion));
    listener->addController(httpController);
}

// CMD - help 
void cmd_help()
{
    cout << endl << "Possible commands: add, delete, list, stop, exit" << endl;
}

// CMD - prints a list of configured cameras
void cmd_list()
{
	int i=0;
	for (struct nvrCameras n:nvrcams)
	{
		cout <<i<<":"<< n.cam->name() <<"-->"<<n.cam->source() << endl;
		i++;
	}
}


// CMD - delets a camera
void cmd_delete()
{
	if (nvrcams.size() == 0)
	{
		cout << "No items to delete.\n\n";
		return;
	}
	cmd_list();
	string sx;
	int x;
	cin.clear(); cin.sync();
	do {cout << "Delete index:"; getline(cin,sx); x=stoi(sx);} while (x > nvrcams.size());
	list<struct nvrCameras>::iterator i = nvrcams.begin();
	while ( i != nvrcams.end())
    {
		if (x==0) break;
		x--;
    }
	
	
	(*i).cam->stop();
	(*i).motion->stop();
	(*i).cam->join();
	cout << "Camera killed\n";
	(*i).motion->join();
	cout << "Camera Motion killed\n";
	nvrcams.erase(i);
  }

// CMD - default handler
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
	cmd_map["delete"] = &cmd_delete;
    
    
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
