This is Samsung's libtuv with support for only Linux and nuttx. Rather than using epoll,eventfd, and 
inotify on Linux, both platforms share a simple poll() based event loop to keep amortized syscall 
overhead to a minimum.

You can find the upstream Samsung project details in their [wiki](https://github.com/pando-project/libtuv/wiki)
