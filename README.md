# qwer
qwer is a funcional network library with little web support.

# Quick Start

> Http server
```cpp
//! @brief multi thread server
int deamon_1() {
      // set log
      LOG_SET_FILE_P("./tmp_log.txt", false);
      LOG_SET_LEVEL("DEBUG");
      LOG_DEBUG(" \n \n in test_multithread");
      // 
      // set http api
      shared_ptr<HttpResponse> x_http_response(new HttpResponse());
      x_http_response->version_ = HttpMsg::HttpVersion ::Http1_1;
      x_http_response->statuscode_ = HttpMsg::HttpStatusCodes::c_200;
      x_http_response->headers_lines_[0] = make_pair("Connection", " close");
      x_http_response->body_ = string("<html><body>") +
              string("<h1>Hello, World!</h1>") +
              string("</body></html>") ;  //!< you can also use FileReader("filename").to_string();
      shared_ptr<HttpSetting> httpsettings(new HttpSetting());
      HttpServer<MultiServer> httpserver(100,
              Ipv4Addr(Ipv4Addr::host2ip_str("localhost"), 8080),
              httpsettings);
      httpsettings->cash_response_of("GET", "/hello.html", move(x_http_response))
              .set_action_of("GET", "/hello.html", [&httpserver]
                     (TCPConnection& this_con, shared_ptr<HttpRequest> req, shared_ptr<HttpResponse> res){ //!< you don't need to modify this res, because res is cashed, (effective consider)
                  //! you can add your header handler and filters here like Nginx provide
                  // your_header_handler(req->headers_lines_);
                  this_con.to_lazy_close();   //!< close this connection after write the content, because your response headers_lines_[0]
               });
      // serve and block until functor return true;
      cout << "begin server" << endl;
      httpserver.Start([](){
                  std::this_thread::sleep_for(10000ms);   //!< sleep of this command thread won't affect threads in pool
                  return true;
              
              });
      cout << "end of server" << endl;
      httpserver.Exit();
  
}       

int main() { return deamon_1(); }
```

# Main featur

0. supporting single thread and multithread
1. supporting http server.

# Install & and Usage

* compile:

        cd ./qwer/qwer_main
        mkdir ./bin
        cd ./bin
        ../runme_instead_of_cmake.sh // or cmake ..
        make && make check // or ctest -V

* add dependency in your CMakeLists.txt easily:

```cmake
        add_subdirectoryy(${QWER_LIBRARY_ROOT})
        include_directories(${QWER_INCLUDES})
```

* using wrk to benchmark "real_example_using_this":

        Running 30s test @ http://127.0.0.1:8080/hello.html
          2 threads and 200 connections
          Thread Stats   Avg      Stdev     Max   +/- Stdev
            Latency    10.23ms   19.15ms 435.35ms   98.98%
            Req/Sec     3.67k     2.77k    8.17k    51.95%
          28798 requests in 30.04s, 2.42MB read
          Socket errors: connect 265, read 28798, write 0, timeout 0
        Requests/sec:    958.50
        Transfer/sec:     82.37KB

<font size=6 color="green">
------ words below this line is not for user ------
</font>

# Develop

* http : 
    * nodejs http-parser [ref](https://github.com/nodejs/http-parser)
    * webmachine [ref](https://github.com/webmachine/webmachine/blob/master/src/webmachine_decision_core.erl), can I do my http decision like this?
        webmachine image [ref](https://github.com/webmachine/webmachine/blob/master/docs/http-headers-status-v3.png)

* web framework:
    * crow
    * silicon
    * middleware [ref](http://expressjs.com/zh-cn/guide/using-middleware.html) what is, how to create one. [ref](http://expressjs.com/zh-cn/guide/writing-middleware.html)

* other : 
    * one reactor per thread(one event-loop per thread) 
    * thread pool with main thread to accept connection, and other thread to handle connections
    * TCP for IPC 
    * message format : json or google protocol buffers
    * thread safe queue for thread communication and data passing

* network library :

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
- [x] try something about corutine, about microservices-architecture : WON't[ref](http://dunkels.com/adam/pt/about.html)
- [x] can I create a object in one thread and use it in another thread? Yes, of course. [ref](https://stackoverflow.com/questions/9697865/what-happens-if-i-call-an-objects-member-function-from-a-different-thread)
- [] try something about reflection
- [] wrap http_parser into a class to use

# Other

1.  cmake config.h, is generated one by cmake:configure_file()
