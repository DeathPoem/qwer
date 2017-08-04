# qwer

We want qwer be a funcional mainly with some object-oriented(or based depends on how many inheritance) network library

# Usage

* do following :

        cd ./qwer/qwer_main
        mkdir ./bin
        cd ./bin
        ../runme_instead_of_cmake.sh // or cmake ..
        make && make check // or ctest -V

* using wrk to benchmark this project :

        Running 30s test @ http://127.0.0.1:8080/hello.html
          2 threads and 200 connections
          Thread Stats   Avg      Stdev     Max   +/- Stdev
            Latency    10.23ms   19.15ms 435.35ms   98.98%
            Req/Sec     3.67k     2.77k    8.17k    51.95%
          28798 requests in 30.04s, 2.42MB read
          Socket errors: connect 265, read 28798, write 0, timeout 0
        Requests/sec:    958.50
        Transfer/sec:     82.37KB

<font size=4 color="blue">
---- ---- ---- ---- ---- dividing line ------ words below this line is not for user ------
</font>

# Develop

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

# Todo list

- [x] gtest error : recipe for target 'test' failed; type make test in /bin
- [x] vim-instant-markdown : install nodejs-legacy fail
- [x] make logging cpp stream style
- [x] running time logging level set
- [x] git third party, gtest
- [] try something about corutine, about microservices-architecture
- [x] can I create a object in one thread and use it in another thread? Yes, of course. [ref](https://stackoverflow.com/questions/9697865/what-happens-if-i-call-an-objects-member-function-from-a-different-thread)
- [] try something about reflection
- [] wrap http_parser into a class to use

# Other

1.  cmake config.h, is generated one by cmake:configure_file()
