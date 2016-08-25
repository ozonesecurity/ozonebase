
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
#define SHOW_FFMPEG_LOG 0
#define EVENT_REC_PATH "nvrcli_events"

#define face_resize_w 1024
#define face_resize_h 768
#define face_refresh_rate 10

#define video_record_w 640
#define video_record_h 480

using namespace std;

// Will hold all cameras and related functions
class  nvrCameras
{
public:
    NetworkAVInput *cam;
    Detector *motion; // keeping two detectors as they can run in parallel
    Detector *face;   
    Recorder *event; // will either store video or images 
    RateLimiter *rate;
    ImageConvert *resize; // not used 

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
    string record;

    cin.clear(); cin.sync();
    
    
    //cout << "camera name (ENTER for default):";
    //getline(cin,name);
    name = "";
    
    cout << "RTSP source (ENTER for default):";
    getline(cin,source);
    cout << "Detection type ([m]otion/[f]ace/[b]oth) (ENTER for default = b):";
    getline (cin, type);
    cout << "Record events? ([y]es/[n]o) (ENTER for default = n):";
    getline (cin, record);

    // Process input, fill in defaults if needed
    if (name.size()==0 )
    {
        string n = to_string(camid);
        camid++;
        name = "cam" + n;
    }
    cout << "Camera name:" << name << endl;
    
    if (source.size() == 0 )
    {
        source = defRtspUrls[nvrcams.size() % MAX_CAMS];
    }
    cout << "Camera source:" << source << endl;
    
    if (record.size() ==0 || (record != "y" && record != "n" ))
    {
        record = "n";
    }
    cout << "Recording will be " << (record=="n"?"skipped":"stored") << " for:" << name << endl;
    
    if (type.size() ==0 || (type != "m" && type != "f" && type != "b"))
    {
        type = "b";
    }
    cout << "Detection type is: ";
    if (type=="f")
    {
        cout << "Face";
    }
    else if (type =="m")
    {
        cout << "Motion";
    }
    else if (type=="b")
    {
        cout << "Face+Motion";
    }
    cout << endl; 
    
    nvrCameras nvrcam;

    // NULLify everything so I know what to delete later
    nvrcam.cam = NULL;
    nvrcam.motion = NULL;
    nvrcam.face = NULL;
    nvrcam.event = NULL;
    nvrcam.rate = NULL;
    nvrcam.resize = NULL;

    nvrcam.cam = new NetworkAVInput ( name, source,"",true );
    if (type == "f")
    {
        nvrcam.face = new FaceDetector( "face-"+name );
        nvrcam.rate = new RateLimiter( "rate-"+name,face_refresh_rate,true );
        nvrcam.rate->registerProvider(*(nvrcam.cam) );
        nvrcam.face->registerProvider(*(nvrcam.rate) );
    }
    else if (type=="m")
    {
        nvrcam.motion = new MotionDetector( "modect-"+name );
        nvrcam.motion->registerProvider(*(nvrcam.cam) );
    }
    else // both
    {
        nvrcam.face = new FaceDetector( "face-"+name );
        nvrcam.rate = new RateLimiter( "rate-"+name,face_refresh_rate,true );
        nvrcam.rate->registerProvider(*(nvrcam.cam) );
        nvrcam.face->registerProvider(*(nvrcam.rate) );
        nvrcam.motion = new MotionDetector( "modect-"+name );
        nvrcam.motion->registerProvider(*(nvrcam.cam) );
    }

    char path[2000];
    snprintf (path, 1999, "%s/%s",EVENT_REC_PATH,name.c_str());
    if (record == "y")
    {
        cout << "Events recorded to: " << path << endl;
    }
    else 
    {
        cout << "Recording will be skipped" << endl;
    }

    mkdir (path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

#if RECORD_VIDEO
    VideoParms* videoParms= new VideoParms( video_record_w, video_record_h );
    AudioParms* audioParms = new AudioParms;
    nvrcam.event = new VideoRecorder(name, path, "mp4", *videoParms, *audioParms);
    if (type=="m")
    { 
        nvrcam.event->registerProvider(*(nvrcam.motion));
    }
    else if (type == "f")
    {
        
        nvrcam.event->registerProvider(*(nvrcam.face));
    }
    else if (type == "b")
    {
        
        cout << "only registering face detection events" << endl;
        nvrcam.event->registerProvider(*(nvrcam.face));
    }
    notifier->registerProvider(*(nvrcam.event));
#else
    nvrcam.event = new EventRecorder( "event-"+name,  path);

    if (type=="m")
    {
        nvrcam.event->registerProvider(*(nvrcam.motion));
    }
    else if (type=="f")
    {
        nvrcam.event->registerProvider(*(nvrcam.face));
    }
    else if (type == "b")
    {
        
        cout << "only registering face detection events" << endl;
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
    else if (type=="f")
    {
        nvrcams.back().rate->start();
        nvrcams.back().face->start();
    }
    else if (type=="b")
    {
        nvrcams.back().motion->start();
        nvrcams.back().rate->start();
        nvrcams.back().face->start();
    }

    if (record == "y")
    {
        nvrcams.back().event->start();
    }
    listener->removeController(httpController);
    httpController->addStream("live",*(nvrcam.cam));
    if (type=="m")
    {
        httpController->addStream("debug",*(nvrcam.motion));
    }
    else if (type =="f")
    {
        httpController->addStream("debug",*(nvrcam.face));
    }
    else if (type == "b")
    {
        httpController->addStream("debug",*(nvrcam.motion));
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

    (*i).event->stop();
    (*i).event->join();
    cout << "Camera  Record killed\n";

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

void monitorStatus(Application app)
{
    for (;;)
    {
        list<nvrCameras>::iterator i = nvrcams.begin();
        while ( i!= nvrcams.end())
        {
            int isTerminated = (*i).cam->ended() + (*i).cam->error();
            if (isTerminated >0)
            {
                cout << "Bad state found for " << (*i).cam->name() << "..deleting..."<<endl;

               
                if ((*i).cam) { cout << "cam kill"<< endl;  (*i).cam->stop(); (*i).cam->join();}
                if ((*i).motion) { cout<<  "motion kill"<< endl;(*i).motion->deregisterAllProviders();(*i).motion->stop(); (*i).motion->join(); }
                if ((*i).face) { cout<<  "face kill"<< endl;(*i).face->deregisterAllProviders();(*i).face->stop(); (*i).face->join(); }
                if ((*i).event) { cout << "event kill"<< endl;notifier->deregisterProvider(*((*i).event)); (*i).event->deregisterAllProviders();(*i).event->stop(); (*i).event->join();  }
                if ((*i).rate) { cout << "rate kill"<< endl; (*i).rate->deregisterAllProviders();(*i).rate->stop(); (*i).rate->join(); }
              
                i = nvrcams.erase(i); // point to next iterator on delete
    
            }
            else
            {
                i++; // increment if not deleted
            }
        }
        
        sleep(5);
    }
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
        transform(command.begin(), command.end(), command.begin(), [](unsigned char c) { return tolower(c); }); // lowercase 
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
    thread t2(monitorStatus,app);
    
    app.run();
    cout << "Never here";
}
