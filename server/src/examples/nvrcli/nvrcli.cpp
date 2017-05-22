
/************************************************************
*
* This is an example of writing a simple command line NVR
* The app is single process, which is often what one needs
* for small systems. oZone can be used to create both
* distributed systems or monolithic systems.
*
*************************************************************/
#include <iostream>
#include <thread>
#include <string>
#include <cctype>
#include <algorithm>
#include <unordered_map>
#include <map>
#include <list>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <mutex>
#include "nvrcli.h"
#include "nvrNotifyOutput.h"

#define MAX_CAMS 10
#define RECORD_VIDEO 1 // 1 if video is on
#define SHOW_FFMPEG_LOG 1
#define EVENT_REC_PATH "nvrcli_events"

#define person_resize_w 1024
#define person_resize_h 768
#define person_refresh_rate 5

#define video_record_w 640
#define video_record_h 480

using namespace std;

// Will hold all cameras and related functions
class  nvrCameras
{
public:
    AVInput *cam;
    Detector *motion; // keeping multiple detectors
    Detector *person;   
    Detector *face;   
    Recorder *event; // will either store video or images 
    RateLimiter *rate;
    LocalFileOutput *fileOut;
    bool scheduleDelete;

};

list <nvrCameras> nvrcams;
int camid=0; // id to suffix to cam-name. always increasing
Listener *listener;
NotifyOutput *notifier;
HttpController* httpController;
Application app;
Options avOptions;
std::mutex mtx;

// default URLs to use if none specified
// feel free to add custom URLs here
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

// traps the ultra-verbose ffmpeg logs. If you want them on enable SHOW_FFMPEG_LOG above
static void avlog_cb(void *, int level, const char * fmt, va_list vl) 
{
#if SHOW_FFMPEG_LOG
    char logbuf[2000];
    vsnprintf(logbuf, sizeof(logbuf), fmt, vl);
    logbuf[sizeof(logbuf) - 1] = '\0';
    cout  << logbuf;
#endif

}

// releases resources of a camera object
void destroyCam (nvrCameras& i)
{
    cout << "waiting for mutex lock..." << endl;
    mtx.lock();
    cout << "got mutex!" << endl;
     if (i.cam) { cout << "removing camera"<< endl;  i.cam->stop(); }
     if (i.motion) { cout<<  "stopping motion"<< endl;i.motion->deregisterAllProviders();i.motion->stop(); }
     if (i.person) { cout<<  "stopping shape detection"<< endl;i.person->deregisterAllProviders();i.person->stop(); }
     if (i.face) { cout<<  "stopping face detection"<< endl;i.face->deregisterAllProviders();i.face->stop(); }
     if (i.event) { cout << "stopping event recorder"<< endl;notifier->deregisterProvider(*(i.event)); i.event->deregisterAllProviders();i.event->stop();}
     if (i.rate) { cout << "stopping rate limiter"<< endl; i.rate->deregisterAllProviders();i.rate->stop(); }
    cout << "all done, mutex released" << endl;
    mtx.unlock();
      
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
    
    name = "";
    cout << "RTSP source (ENTER for default, or 'osx' for OSX faceTime camera):";
    getline(cin,source);
    cout << "Detection type ([m]otion/[f]ace/[p]erson/[a]ll/[n]one) (ENTER for default = a):";
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
    
    if (type.size() ==0 || (type != "m" && type != "f" && type != "a" && type !="p" && type !="n"))
    {
        type = "a";
    }


    cout << "Detection type is: ";
    if (type=="f"){cout << "Face";}
    else if (type =="m"){cout << "Motion";}
    else if (type == "p"){cout << "Person";}
    else if (type=="a"){cout << "Face+Motion+Person";}
    else if (type=="n"){cout << "None";}
    cout << endl; 
    
    nvrCameras nvrcam;

    // NULLify everything so I know what to delete later
    nvrcam.cam = NULL;
    nvrcam.motion = NULL;
    nvrcam.person = NULL;
    nvrcam.face = NULL;
    nvrcam.event = NULL;
    nvrcam.rate = NULL;
    nvrcam.fileOut = NULL;
    nvrcam.scheduleDelete = false;

    Options camOptions ;

    if (source == "osx")
    {
    	cout << "Setting correct AV sources for facetime";
    	camOptions.add("format","avfoundation");
    	camOptions.add("framerate","30");
    	source="0";
    }
    nvrcam.cam = new AVInput ( name, source,camOptions );
    if (type == "f") // only instantiate face recog
    {
    	Options faceOptions;
    	faceOptions.set( "method", "hog" );
    	faceOptions.set( "dataFile", "shape_predictor_68_face_landmarks.dat" );
    	faceOptions.set( "markup", FaceDetector::OZ_FACE_MARKUP_ALL );
        nvrcam.face = new FaceDetector( "face-"+name,faceOptions);
        nvrcam.rate = new RateLimiter( "rate-"+name,person_refresh_rate,true );
        nvrcam.rate->registerProvider(*(nvrcam.cam), gQueuedVideoLink );
        nvrcam.face->registerProvider(*(nvrcam.rate),gQueuedVideoLink );
    }
    else if (type=="p") // only instantiate people recog
    {
    	
        nvrcam.person = new ShapeDetector( "person-"+name, "person.svm" );
        nvrcam.rate = new RateLimiter( "rate-"+name,person_refresh_rate, true );
        nvrcam.rate->registerProvider(*(nvrcam.cam), gQueuedVideoLink );
        nvrcam.person->registerProvider(*(nvrcam.rate), gQueuedVideoLink);
}
    else if (type=="m") // only instantate motion detect
    {
        nvrcam.motion = new MotionDetector( "modect-"+name );
        nvrcam.motion->registerProvider(*(nvrcam.cam) );
    }
    else if (type!="n") // face/motion/person - turn them all on
    {
    	Options faceOptions;
    	faceOptions.set( "method", "hog" );
        //faceOptions.set( "method", "cnn" );
    	faceOptions.set( "dataFile", "shape_predictor_68_face_landmarks.dat" );
    	faceOptions.set( "markup", FaceDetector::OZ_FACE_MARKUP_ALL );
        nvrcam.face = new FaceDetector( "face-"+name,faceOptions);

        nvrcam.person = new ShapeDetector( "person-"+name, "person.svm" );

        //nvrcam.fileOut = new LocalFileOutput( "file-"+name, "/tmp" );
        nvrcam.rate = new RateLimiter( "rate-"+name,person_refresh_rate,true );
        nvrcam.rate->registerProvider(*(nvrcam.cam),gQueuedVideoLink );
        nvrcam.person->registerProvider(*(nvrcam.rate),gQueuedVideoLink );
        nvrcam.face->registerProvider(*(nvrcam.rate),gQueuedVideoLink );
        //nvrcam.fileOut->registerProvider(*(nvrcam.face) );
        nvrcam.motion = new MotionDetector( "modect-"+name );
        nvrcam.motion->registerProvider(*(nvrcam.cam) );
    }

    //setup path for events recording
    char path[2000];
    snprintf (path, 1999, "%s/%s",EVENT_REC_PATH,name.c_str());
    if (record == "y")
    {
         mkdir (path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    #if RECORD_VIDEO
        VideoParms* videoParms= new VideoParms( video_record_w, video_record_h );
        AudioParms* audioParms = new AudioParms;
        nvrcam.event = new VideoRecorder(name, path, "mp4", *videoParms, *audioParms, 30);
    #else
        nvrcam.event = new EventRecorder( "event-"+name,  path,30);
    #endif
        if (type=="m")
        { 
            nvrcam.event->registerProvider(*(nvrcam.motion));
        }
        else if (type == "f")
        {
            
            nvrcam.event->registerProvider(*(nvrcam.face));
        }
        else if (type == "p")
        {
            nvrcam.event->registerProvider(*(nvrcam.person));
        }
        else if (type == "a")
        {
            
            cout << "only registering person detection events" << endl;
            nvrcam.event->registerProvider(*(nvrcam.person));
        }
        notifier->registerProvider(*(nvrcam.event));
        notifier->start();
       cout << "Events recorded to: " << path << endl;
    }
    else 
    {
        cout << "Recording will be skipped" << endl;
    }

    nvrcams.push_back(nvrcam); // add to list
    
    cout << "Added:"<<nvrcams.back().cam->name() << endl;
    cout << nvrcams.back().cam->source() << endl;

    //nvrcams.back().cam->start();
    nvrcam.cam->start();

    if (nvrcams.back().motion != NULL){nvrcams.back().motion->start(); cout << "starting motion"<< endl;}
    if (nvrcams.back().rate != NULL) {nvrcams.back().rate->start(); cout << "starting rate" << endl;}
    if (nvrcams.back().person != NULL) {nvrcams.back().person->start(); cout << "starting person" << endl;}
    if (nvrcams.back().face != NULL) {nvrcams.back().face->start(); cout << "starting face" << endl;}
    //if (nvrcams.back().fileOut != NULL) {nvrcams.back().fileOut->start();}

    if (record == "y")
    {
        nvrcams.back().event->start();
        cout << "starting events" << endl;
    }
    listener->removeController(httpController);
    httpController->addStream("live",*(nvrcam.cam));
    if (type=="m")
    {
        httpController->addStream("debug",*(nvrcam.motion));
        cout << "starting motion http" << endl;
    }
    else if (type == "p")
    {
        httpController->addStream("debug",*(nvrcam.person));
        cout << "starting person http" << endl;
    }
    else if (type =="f")
    {
        httpController->addStream("debug",*(nvrcam.face));
        cout << "starting face http" << endl;
    }
    else if (type == "a")
    {
        cout << "starting motion+person+face http" << endl;
        httpController->addStream("debug",*(nvrcam.motion));
        httpController->addStream("debug",*(nvrcam.person));
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
    unsigned int x;
    cin.clear(); cin.sync();
    do {cout << "Delete index:"; getline(cin,sx); x=stoi(sx);} while (x > nvrcams.size());
    list<nvrCameras>::iterator i = nvrcams.begin();
    while ( i != nvrcams.end()) // iterate to that index selected
    {
        if (x==0) break;
        x--;
        i++;
    }
    
    (*i).scheduleDelete = true;
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


// monitors active camera list and removes them if terminated
void monitorStatus(Application app)
{
    for (;;)
    {
        list<nvrCameras>::iterator i = nvrcams.begin();
        while ( i!= nvrcams.end())
        {
            int isTerminated = (*i).cam->ended() + (*i).cam->error();
            if (isTerminated >0 || (*i).scheduleDelete == true)
            {
                cout << "Bad state found for " << (*i).cam->name() << "..deleting..."<<endl;

               
                (*i).scheduleDelete = false;
                destroyCam(*i);        
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
    
    avOptions.add("realtime",true); // if file, honor fps, don't blaze through
    avOptions.add("loop",true); // if file, loop on end

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
