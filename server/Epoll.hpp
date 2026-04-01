/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilazar <ilazar@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/23 19:16:18 by ilazar            #+#    #+#             */
/*   Updated: 2025/12/12 19:13:56 by ilazar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

class Epoll {
    
    private:
        int _epfd;
        Epoll(const Epoll &);
        Epoll& operator=(const Epoll &);

    public:
        Epoll(int maxEvents);
        ~Epoll();

        
        bool    addFd(int fd, unsigned int events, void* ptr); //Add an Fd to the epoll interests list
        void    delFd(int fd);                      //remove an Fd from epoll interest list
        int     wait(struct epoll_event *events, int maxEvents, int timeout); //Wait for events
        bool    modFd(int fd, unsigned int events, void* ptr); //Modify interest events for an existing Fd
        void    setModWritable(int fd, void* ptr);
        void    setModNonWritable(int fd, void* ptr);

        
        int     getFd() const; //Get epoll fd

        static void     setNonBlocking(int fd); //Make a socket non-blocking
};