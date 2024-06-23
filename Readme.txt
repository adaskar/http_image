required libraries 
    libuv1-dev
    libmagickwand-dev
    libcurl4
    libcurl4-openssl-dev
    
to install required libraries, run
sudo apt install libuv1-dev libmagickwand-dev libcurl4 libcurl4-openssl-dev

you can use Visual Code to open the project
    to build the project using Visual Code, press ctl+shift+b (Terminal -> Run Build Task)
    to run the project via debugger just press f5 (Run -> Start Debugging)

list of available operations:
    crop X:10 Y:11 W:100 H:101
    http://localhost:8080/v1/crop:10x11_100x101/url:upload.wikimedia.org/wikipedia/en/a/a9/Example.jpg

    resize W:100 H:101
    http://localhost:8080/v1/resize:100x101/url:upload.wikimedia.org/wikipedia/en/a/a9/Example.jpg

    grayscale
    http://localhost:8080/v1/grayscale/url:upload.wikimedia.org/wikipedia/en/a/a9/Example.jpg

    rotate Angle:60
    http://localhost:8080/v1/rotate:60/url:upload.wikimedia.org/wikipedia/en/a/a9/Example.jpg