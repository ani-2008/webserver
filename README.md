# A simple webserver #

A lightweight, single threaded HTTP web server built from scratch using C and POSIX sockets. 
Developed for learning about low-level network programming.

## Features ##
- listens on localhost port 6969
- parses GET requests, and serves the requested file
- Returns proper HTTP codes
- serves static files from local directory

## Running ##

```bash

git clone git@github.com:ani-2008/webserver.git 
cd webserver

make 

./bin/server 

```

## Testing ##

Go to browser and type `http:/localhost:6969/` and it will serve `pages/index.html` directory.

Any other invalid files which is not in `pages/` gets requested, a 404 page will be served

if there is any other request other than `GET` like `POST` from `pages/post.html` will result in `405 Method Not Allowed`

if files like `pages/404.html` is missing then results in `500 Internal Server Error` 

## To-Do ##
- [ ] Multi-threading
- [ ] MIME detection

