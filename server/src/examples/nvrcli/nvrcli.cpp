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
Listener listener;
HttpController* httpController;
Application app;
int cam_ndx=-1;


// adds a new camera and motion detector
void cmd_add()
{
    string name;
    string source;
    cout << "camera name:";
    cin >> name;
    //cout << "RTSP source:";
    //cin >> source;
    // fixme- hardcoded
    cam[0] = new NetworkAVInput ( name, "rtsp://170.93.143.139:1935/rtplive/0b01b57900060075004d823633235daa" );
    cam_ndx++;
    cout << "Adding:"<<cam[cam_ndx]->name() << endl;
    cout << cam[cam_ndx]->source() << endl;
    app.addThread( cam[cam_ndx] );
    cam[cam_ndx]->start();

    // motion detect for cam
    motion[cam_ndx] = new MotionDetector( "modect-"+name );
    motion[cam_ndx]->registerProvider(*cam[cam_ndx] );
    app.addThread( motion[cam_ndx] );
    motion[cam_ndx]->start();

    // stop the listener, add new camera, restart
    listener.stop();
    //listener.removeController(httpController);
    httpController->addStream("watch-"+name,*cam[cam_ndx]);
    //listener.addController(httpController);
    listener.start();
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
        cin >> command;
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
    debugInitialise( "starter-example", "", 0 );
    cout << " \n---------------------- NVRCLI ------------------\n"
             " Type help to get started\n"
             " ------------------------------------------------\n\n";

    Info( "Starting" );

    avInit();

    //fixme: convert this to a list later
    for (int i=0; i<MAX_CAMS; i++) { cam[i] = NULL; motion[i] = NULL; }
    
    httpController = new HttpController( "watch", 9292 );
    listener.addController( httpController );
    app.addThread( &listener );
    thread t1(cli,app);
    app.run();
    cout << "Never here";
}
