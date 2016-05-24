####What

The client framework will be an Angular2+ionic2 framework that will serve as a shell for developers wanting to build their own mobile apps. It will also be possible to run the same code on desktops, via Electron.

####Organization
The client framework will come with the following:
* A complete shell app with a UI that can run on Android and iOS phones (and likely Win too)
* In its unaltered form, when you start the app, it will display screens similar to zmNinja - monitors/montage/events/etc.
* The menu will by default be a side-menu style app (like facebook and zmNinja)
* When it starts up, it will list dummy events, dummy monitor feeds 

####Eh?

This is a framework. The goal is for the developer to use a ready shell and implement their own UI. So lets talk about:

####Customization

This section will discuss what the user can customize

#####Components

The client framework will provide "Components" which comprise of a template and a controller. The template cotrols the UI and the Controller controls the business logic of the controller.  templates are HTML while controllers are Javascript

The Controllers will be:
* Events
* Montage
* Cameras
* Notifications
* Timeline
* Database

Each controller will have callbacks and invocation points which can (and should) be overriden by the developer. Each controller will have well defined interfaces which should be implemented by the developer.

Most controllers will have ready to use templates. the templates decide how the data of the controller will be shown on screen. The templates will be angular templates and will be easy to customize.

For example, here is how a developer may want to extend the concept of events:

This is a sample code for the proposed EventController

```javascript
	var to = moment();
	var from = moment().subtract(1,'day');
	ozoneClient.getEvents(from,to)
	.then (function (success) {
		$scope.events = success.data;
	},
		function (error) {
			ozoneClient.util.showBanner('Error retrieving events');
			ozoneClient.util.log ('Error'+JSON.stringify(error));
	});
```
	



