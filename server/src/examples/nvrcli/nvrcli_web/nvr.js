angular.module('nvrcli', [])
   .controller('nvrcliController', function($scope) {
        // change the IP address to point to your system
       $scope.baseurl = "http://127.0.0.1:9292";
       $scope.w = "400px";
});
