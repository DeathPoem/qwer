# qwer

We want qwer be a funcional mainly with some object-oriented(or based depends on how many inheritance) network library

# Usage

* do following :

        cd ./qwer/qwer_main
        mkdir ./bin
        cd ./bin
        ../runme_instead_of_cmake.sh 
        make 
        make check (or ctest -V)

# develop

* http : 
    * nodejs http-parser [ref](https://github.com/nodejs/http-parser)
    * webmachine [ref](https://github.com/webmachine/webmachine/blob/master/src/webmachine_decision_core.erl), can I do my http decision like this?
        webmachine image [ref](https://github.com/webmachine/webmachine/blob/master/docs/http-headers-status-v3.png)

* network architecture : 
    * one reactor per thread(one event-loop per thread) 
    * thread pool with main thread to accept connection, and other thread to handle connections
    * TCP for IPC 
    * message format : json or google protocol buffers
    * thread safe queue for thread communication and data passing

* More Reference :

    * libevent --> reactor
    * asio boost[c++11 example](http://www.boost.org/doc/libs/master/doc/html/boost_asio/examples/cpp11_examples.html) --> proactor
    * muduo [github](https://github.com/chenshuo/muduo)
    * handy [github](https://github.com/yedf/handy)
    * [Proactor](www.cs.wustl.edu/~schmidt/PDF/proactor.pdf)
    * Design.Pattern-orented software architecture Vol2
        * JAWS p35
        * Reactor p154
    * ZMQ [ref](http://blog.jobbole.com/19647/) what you should know about, when implement a error sensitive project with cpp

# todo list

- [ ] gtest error : recipe for target 'test' failed; type make test in /bin
- [ ] vim-instant-markdown : install nodejs-legacy fail
- [] make logging cpp stream style
- [] running time logging level set
- [x] git third party, gtest
- [] try something about corutine, about microservices-architecture
- [x] can I create a object in one thread and use it in another thread? Yes, of course. [ref](https://stackoverflow.com/questions/9697865/what-happens-if-i-call-an-objects-member-function-from-a-different-thread)
- [] try something about reflection

# Pitfall list 

1.  cmake config.h, is generated one by cmake:configure_file()
