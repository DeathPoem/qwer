# qwer

# Usage

* do following :
        
        cd /bin
        cmake ..
        make 
        make test

# develop

* webmachine [ref](https://github.com/webmachine/webmachine/blob/master/src/webmachine_decision_core.erl), can I do my http decision like this?
    webmachine image [ref](https://github.com/webmachine/webmachine/blob/master/docs/http-headers-status-v3.png)

* architecture : 
    * one reactor per thread(one event-loop per thread) 
    * thread pool with main thread 
    * TCP for IPC 
        * message format : json or google protocol buffers
    * message queue for thread communication

* Reference :

    * [Proactor](www.cs.wustl.edu/~schmidt/PDF/proactor.pdf)
    * Design.Pattern-orented software architecture Vol2
        * JAWS p35
        * Reactor p154

# todo list

- [x] gtest error : recipe for target 'test' failed
