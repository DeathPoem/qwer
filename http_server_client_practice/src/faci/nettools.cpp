#include "nettools.h"
#include "facility.h"

namespace my_http {

    Ipv4Addr::Ipv4Addr(const struct sockaddr_in& addr_arg) : addr_(addr_arg) {}

    int Ipv4Addr::hostname_2_ip(char* hostname, char* ip) {
        struct hostent *he;
        struct in_addr **addr_list;
        if ((he = gethostbyname(hostname)) == NULL) {
            // get the host info
            int iii = 1;
            LOG_ERROR("gethostbyname,nomeaning =%d", iii);
            return 1;
        }
        addr_list = (struct in_addr **)he->h_addr_list;
        for(int i = 0; addr_list[i] != NULL; i++) {
            //Return the first one;
            strcpy(ip , inet_ntoa(*addr_list[i]) );
            return 0;
        }
        return 1;
    }

    string Ipv4Addr::host2ip_str(const string& host) {
        char ipcstr[100], hostnamecstr[100];
        strcpy(hostnamecstr, host.c_str());
        if (Ipv4Addr::hostname_2_ip(hostnamecstr, ipcstr) == 0) {
            return string(ipcstr);
        } else {
            return "";
        }
    }

    Ipv4Addr::Ipv4Addr(string host, int port) {
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        string ip_str = Ipv4Addr::host2ip_str(host);
        if (host == "") {
            addr_.sin_addr.s_addr = INADDR_ANY;
        } else if (!ip_str.empty()) {
            addr_.sin_addr.s_addr = htons(stoi(ip_str));
        } else {
            LOG_ERROR("not able to resolve hostname 2 ip");
        }
    }

    string Ipv4Addr::get_ip_str() {
        int ip_int = addr_.sin_addr.s_addr;
        return to_string(ip_int);
    }
} /* my_http */ 
