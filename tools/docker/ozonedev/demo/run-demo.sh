#/bin/bash
export PATH"=$PATH:$HOME/demo/node_modules/.bin"
forever start -a -l /dev/null -o /dev/null -e /dev/null --uid "binary" --command "/bin/bash -c" /home/ozone/install/bin/starter_example
forever -a -l /dev/null -o /dev/null -e /dev/null --uid "web" /home/ozone/demo/demo.js
