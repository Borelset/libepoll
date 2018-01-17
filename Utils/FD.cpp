//
// Created by borelset on 1/15/18.
//

#include <zconf.h>
#include "FD.h"

using namespace Utils;

FD::FD(int fd):
        mFd(fd)
{

}

FD::~FD() {
    close(mFd);
}

int FD::getFd() {
    return mFd;
}