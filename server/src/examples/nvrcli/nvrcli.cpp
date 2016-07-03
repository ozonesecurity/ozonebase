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
	for (int i=0; i<MAX_CAMS; i++) { cam[i] = NULL; motion[i] = NULL; }
	unordered_map<std::string, std::function<void()>> cmd_map;
	cmd_map["help"] = &cmd_help;
	
	
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


   // Two RTSP sources 
    cam[0] = new NetworkAVInput ( "cam1", "rtsp://170.93.143.139:1935/rtplive/0b01b57900060075004d823633235daa" );
	cout << cam[0]->name() << endl;
	cout << cam[0]->source() << endl;
    app.addThread( cam[0] );

	// motion detect for cam1
	motion[0] = new MotionDetector( "modectcam1" );
  	motion[0]->registerProvider(*cam[0] );
   	app.addThread( motion[0] );


    	app.addThread( &listener );

    httpController = new HttpController( "watch", 9292 );
    httpController->addStream("watchcam1",*cam[0]);

	
    	listener.addController( httpController );

	thread t1(cli,app);
	app.run();
   	cout << "Never here";
}
