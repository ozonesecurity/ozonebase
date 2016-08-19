
/************************************************************
*
* This is an example of writing a simple command line NVR
* The app is single process, which is often what one needs
* for small systems. oZone can be used to create both
* distributed systems (like ZoneMinder v1) or monolithic 
* systems (not possible in ZoneMinder v1). 
*
*************************************************************/


#include <iostream>
#include <thread>
#include <string>
#include <cctype>
#include <algorithm>
#include <unordered_map>
#include <list>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include "ozone.h"
#include "nvrNotifyOutput.h"

#define MAX_CAMS 10
#define RECORD_VIDEO 1 // 1 if video is on
#define SHOW_FFMPEG_LOG 1 
#define EVENT_REC_PATH "nvrcli_events"

using namespace std;

// Will hold all cameras and related functions
class  nvrCameras
{
public:
    NetworkAVInput *cam;
    MotionDetector *motion; 
    FaceDetector *face;   
    EventRecorder *event; // used if RECORD_VIDEO = 0
    VideoRecorder *movie; // used if RECORD_VIDEO = 1

};

list <nvrCameras> nvrcams;
int camid=0; // id to suffix to cam-name. always increasing
Listener *listener;
NotifyOutput *notifier;
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

static void avlog_cb(void *, int level, const char * fmt, va_list vl) 
{
#if SHOW_FFMPEG_LOG
    char logbuf[2000];
    vsnprintf(logbuf, sizeof(logbuf), fmt, vl);
    logbuf[sizeof(logbuf) - 1] = '\0';
    cout  << logbuf;
#endif

}

// Adds a camera
void cmd_add()
{

    if (nvrcams.size() == MAX_CAMS)
    {
        cout << "Cannot add any more cams!\n\n";
        return;
    }

    string name;
    string source;
    string type;

    cin.clear(); cin.sync();
    cout << "camera name (ENTER for default):";
    getline(cin,name);
    cout << "RTSP source (ENTER for default):";
    getline(cin,source);
    cout << "Detection type (m/f) (ENTER for default):";
	getline (cin, type);

    if (type.size() ==0 || (type != "m" && type != "f"))
  	{
		type = "m";
	}
	cout << "Detection type is: " << (type == "m" ? "Motion Detect":"Face Detect") << endl;

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
    
    
    nvrCameras nvrcam;
    nvrcam.cam = new NetworkAVInput ( name, source,"",true );
	if (type == "f")
	{
    	nvrcam.face = new FaceDetector( "modect-"+name );
    	nvrcam.face->registerProvider(*(nvrcam.cam) );
	}
	else
	{
    	nvrcam.motion = new MotionDetector( "modect-"+name );
    	nvrcam.motion->registerProvider(*(nvrcam.cam) );
	}

    char path[2000];
    snprintf (path, 1999, "%s/%s",EVENT_REC_PATH,name.c_str());
    cout << "Events recorded to: " << path << endl;

    mkdir (path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

#if RECORD_VIDEO
    VideoParms* videoParms= new VideoParms( 640, 480 );
    AudioParms* audioParms = new AudioParms;
    nvrcam.movie = new VideoRecorder(name, path, "mp4", *videoParms, *audioParms);
    if (type=="m")
	{ 
		nvrcam.movie->registerProvider(*(nvrcam.motion));
	}
	else
	{
		
		nvrcam.movie->registerProvider(*(nvrcam.face));
	}
    notifier->registerProvider(*(nvrcam.movie));
#else
    nvrcam.event = new EventRecorder( "event-"+name,  path);

	if (type=="m")
	{
    	nvrcam.event->registerProvider(*(nvrcam.motion));
	}
	else
	{
    	nvrcam.event->registerProvider(*(nvrcam.face));
	}
    notifier->registerProvider(*(nvrcam.event));

#endif

    notifier->start();
    nvrcams.push_back(nvrcam); // add to list
    
    cout << "Added:"<<nvrcams.back().cam->name() << endl;
    cout << nvrcams.back().cam->source() << endl;

    nvrcams.back().cam->start();
	if (type=="m")
	{
    	nvrcams.back().motion->start();
	}
	else
	{
		nvrcams.back().face->start();
	}
#if RECORD_VIDEO
    nvrcams.back().movie->start();
#else
    nvrcams.back().event->start();
#endif
    listener->removeController(httpController);
    httpController->addStream("live",*(nvrcam.cam));
	if (type=="m")
	{
    	httpController->addStream("debug",*(nvrcam.motion));
	}
	else
	{
		httpController->addStream("debug",*(nvrcam.face));
	}
    listener->addController(httpController);
}

// CMD - help 
void cmd_help()
{
    cout << endl << "Possible commands: add, delete, list, stop, quit" << endl;
}

// CMD - prints a list of configured cameras
void cmd_list()
{
    int i=0;
    for (nvrCameras n:nvrcams)
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
    list<nvrCameras>::iterator i = nvrcams.begin();
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
#if RECORD_VIDEO
    (*i).movie->stop();
    (*i).movie->join();
    cout << "Camera Movie Record killed\n";

#else
    (*i).event->stop();
    (*i).event->join();
    cout << "Camera Image Record killed\n";
#endif
    nvrcams.erase(i);
}

void cmd_quit()
{
    cout << endl << "Bye."<<endl<<endl;
    exit(0);
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
    cmd_map["quit"] = &cmd_quit;
    
    
    string command;
    for (;;) 
    { 
        cin.clear(); cin.sync();
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
    dbgInit( "nvrcli", "", 5 );
    cout << " \n---------------------- NVRCLI ------------------\n"
             " Type help to get started\n"
             " ------------------------------------------------\n\n";

    Info( "Starting" );

    av_log_set_callback(avlog_cb);
    avInit();
    mkdir (EVENT_REC_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    listener = new Listener;
    httpController = new HttpController( "watch", 9292 );
    listener->addController( httpController );
    app.addThread( listener );

    notifier = new NotifyOutput("notifier");
    app.addThread(notifier);

    thread t1(cli,app);
    app.run();
    cout << "Never here";
}
