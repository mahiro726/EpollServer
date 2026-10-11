#pragma once

#include <iostream>
#include <sys/epoll.h>
#include "Logger.hpp"
#include "InetAddr.hpp"
#include "Socket.hpp"

using namespace NS_LOG_MODULE;

static const int defaultport = 8080;
static const int ign_size = 256;
static const int revs_num = 64;

class EpollServer
{
public:
    EpollServer(uint16_t port = defaultport)
    :_port(port),
     _listensock(std::make_unique<TcpSocket>()),
     _epfd(-1),
     _quit(false)
    {
        // 创建listensockfd
        _listensock->BuildTcpSocketMethod(_port);
        LOG(LogLevel::DEBUG) << "服务端创建 listensock 成功, sockfd is : " << _listensock->GetSockfd();
        
        // 创建Epoll模型
        _epfd = epoll_create(ign_size);
        if (_epfd < 0)
        {
            LOG(LogLevel::FATAL) << "create epoll error, epfd: " << _epfd;
            return;
        }
        LOG(LogLevel::INFO) << "create epoll success, epfd is : " << _epfd;

        // 把 listensockfd 添加到内核 epoll 模型中，让 OS 帮你关心
        struct epoll_event ev;
        ev.data.fd = _listensock->GetSockfd();
        ev.events = EPOLLIN;
        int n = epoll_ctl(_epfd, EPOLL_CTL_ADD, _listensock->GetSockfd(), &ev);
        if (n == 0)
        {
            LOG(LogLevel::INFO) << "add listen sock to epoll success……";
        }
    }

    void Listener()
    {
        InetAddr clientaddr;
        int newsockfd = _listensock->Accept(clientaddr);
        if (newsockfd < 0)
        {
            return;
        }
        // 获取新链接成功
        LOG(LogLevel::INFO) << "accept success, new sockfd: " << newsockfd << " " << clientaddr.ToString();

        // 将新链接托管给epoll，添加到rb中就可以，比 select 和 poll 简单，不需要遍历找位置，直接插进去就行了
        struct epoll_event ev;
        ev.data.fd = newsockfd;
        ev.events = EPOLLIN;
        int n = epoll_ctl(_epfd, EPOLL_CTL_ADD, newsockfd, &ev);
        (void)n;
    }

    void IOService(int sockfd)
    {
        char inbuffer[1024];
        ssize_t n = recv(sockfd, inbuffer, sizeof(inbuffer)-1, 0); // 读取不会被阻塞
        if (n > 0)
        {
            inbuffer[n] = 0;
            LOG(LogLevel::INFO) << "inbuffer: " << inbuffer;
            std::string echo_string = "echo # ";
            echo_string += inbuffer;
            
            send(sockfd, echo_string.c_str(), echo_string.size(), 0); 
        }
        else if(n == 0)
        {
            LOG(LogLevel::INFO) << "client quit, sockfd: " << sockfd;
            // 不让Epoll关心fd
            // 坑：epoll_ctl 要删除对特定fd的事件关心，前提是fd在系统中
            int n = epoll_ctl(_epfd, EPOLL_CTL_DEL, sockfd, nullptr);
            (void)n; 
            // close fd
            close(sockfd);
        }
        else
        {

        }
    }

    void HandlerEvent(struct epoll_event revs[], int n)
    {
        for(int i = 0; i < n; i++)
        {
            int sockfd = revs[i].data.fd;
            uint32_t revents = revs[i].events;
            if (revents & EPOLLIN)
            {
                // 读时间就绪
                // 新链接到来，也是读事件就绪
                if (sockfd == _listensock->GetSockfd())
                {
                    // 获取新链接
                    Listener();
                }
                else
                {
                    // 普通文件就绪
                    IOService(sockfd);
                }
            }
            else if (revents & EPOLLOUT)
            {
                // 写事件就绪
            }
            else if (revents & EPOLLERR)
            {
                // 异常事件
            }
        }
    }

    void Loop()
    {
        int timeout = 1000;
        while(!_quit)
        {
            // epoll_wait
            struct epoll_event revs[revs_num];
            int n = epoll_wait(_epfd, revs, revs_num, timeout);
            if (n > 0)
            {
                // 有事件就绪了
                LOG(LogLevel::DEBUG) << "Event Ready!";
                HandlerEvent(revs, revs_num);
            }
            else if(n == 0)
            {
                LOG(LogLevel::INFO) << "timeout ……";
            }
            else
            {
                LOG(LogLevel::ERROR) << "epoll_wait error";
                break;
            }
        }
    }

    ~EpollServer()
    {}
private:
    std::unique_ptr<Socket> _listensock;
    uint16_t _port; 
    int _epfd;
    bool _quit;
};