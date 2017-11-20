# Minimalist HTTP server

A very simple HTTP server written in pure C, as an *OS & networking* course project.
It has been co-developed with Hugo M.

## Instructions
#### Requirements
The server expects a Unix-like environment, and the cache uses `file` and `gzip` programs.
`make` is required for building the server; and it uses `clang` compiler, though this can be modified in the `Makefile` file.

#### Compiling
Run `make` in the root directory to build the server.
The resulting `webserver` binary will be placed into the `build` directory.

Run `make clean` to remove built files.

#### Starting the server
Run `./build/webserver` in the root directory to set up and start the server.
By default, it uses port 4242, and consider the `www` directory as the root directory of the server.

*You can then try to load `http://localhost:4242/test.html` for a small (French) demo webpage!*

#### Closing the server
There is no better way to close the server than to send it a signal using `CTRL + C` in your terminal.

Note that the server registers an *atexit* function for cleaning stuff up, which is always called if your close it this way.
It is mainly used for cleaning up internal data structures and actually disconnecting clients and *politely* closing the TCP connection.

#### Cleaning
Run `make clean` to clean up the stuff which has been previously built.


## Goals and features
It currently **compile and works** (yay!), but is far from optimized, and only implements a very small subset of the HTTP/1.1 norm (according to [RFC 2616](https://www.ietf.org/rfc/rfc2616.txt)). Our goal mainly was to study low-level Unix options for developping a web server, not to make the fastest server ever.

However, it has been coded with more advanced features in mind, like:
* Using port 80;
* A better support of the HTTP/1.1 norm;
* Sharing HTTP message processing between threads;
* A dynamic caching policy).
But we did not have time to implement them all.
