<html>

<head>
    <!-- Latest compiled and minified CSS -->
    <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/css/bootstrap.min.css" integrity="sha384-1q8mTJOASx8j1Au+a5WDVnPi2lkFfwwEAa8hDDdjZlpLegxhjVME1fgjWPGmkzs7" crossorigin="anonymous">

    <!-- Optional theme -->
    <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/css/bootstrap-theme.min.css" integrity="sha384-fLW2N01lMqjakBkx3l/M9EahuwpSfeNvV63J5ezn3uZzapT0u7EYsXMjQV+0En5r" crossorigin="anonymous">

    <script src="http://ajax.googleapis.com/ajax/libs/angularjs/1.4.8/angular.min.js"></script>
    <!-- Latest compiled and minified JavaScript -->
    <script src="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/js/bootstrap.min.js" integrity="sha384-0mSbJDEHialfmuBBQP6A4Qrprq5OVfW37PRR3j5ELqxss1yVqOtnepnHVP9aJ7xS" crossorigin="anonymous"></script>
    <script src="nvr.js"></script>
</head>


<body>
    <div ng-app="nvrcli" ng-controller="nvrcliController">
        <nav class="navbar navbar-inverse">
            <div class="container-fluid">
                <div class="navbar-header">
                    <a class="navbar-brand" href="http://ozone.network">oZone.Network</a>
                </div>
                <ul class="nav navbar-nav">
                    <li class="active"><a href="#">Demo</a></li>
                    <li><a href="https://github.com/ozonesecurity/ozonebase/blob/master/server/src/examples/nvrcli/nvrcli.cpp">Server Source Code</a></li>
                </ul>
            </div>
        </nav>
        <div class="container">
            <div class="page-header">
                <h1>oZone cli NVR - start example</h1>
            </div>


          
            <span data-ng-repeat="i in [0,1,2,3,4,5,6,7,8,9]">
                      <!-- live frames -->
                    <div class="row">
                        <div class="border col-lg-6">
                            <figure class="figure">
                                <figcaption class="bg-success figure-caption text-xs-right">Camera {{i}}: Live Feed</figcaption>
                                <img class="figure-img img-fluid" width="{{w}}" src="{{baseurl}}/live/cam{{i}}" />
                            </figure>
                        </div>
                          <!-- debug frames -->
                        <div class="border col-lg-6">
                            <figure class="figure">
                                <figcaption class="bg-success figure-caption text-xs-right">Camera {{i}}: MotionDect Feed</figcaption>
                                <img class="figure-img img-fluid" width="{{w}}" src="{{baseurl}}/debug/modect-cam{{i}}" />
                            </figure>
                        </div>
                        <div class="border col-lg-6">
                            <figure class="figure">
                                <figcaption class="bg-success figure-caption text-xs-right">Camera {{i}}: FaceDect Feed</figcaption>
                                <img class="figure-img img-fluid" width="{{w}}" src="{{baseurl}}/debug/face-cam{{i}}" />
                            </figure>
                       </div>
                       <div class="border col-lg-6">
                            <figure class="figure">
                                <figcaption class="bg-success figure-caption text-xs-right">Camera {{i}}: PersonDect Feed</figcaption>
                                <img class="figure-img img-fluid" width="{{w}}" src="{{baseurl}}/debug/person-cam{{i}}" />
                            </figure>
                       </div>
                    </div>
               </span>
        </div>
    </div>
</body>

</html>
