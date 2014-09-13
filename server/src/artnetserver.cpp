/*
 * Art-Net UDP server for Fadecandy
 *
 * Copyright (c) 2014 Micah Elizabeth Scott
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "artnetserver.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

ArtNetServer::ArtNetServer(OPC::callback_t opcCallback, void *context, bool verbose)
    : mOpcCallback(opcCallback), mUserContext(context), mThread(0), mVerbose(verbose)
{}

bool ArtNetServer::start()
{
    mThread = new tthread::thread(threadFunc, this);
    return mThread != 0;
}

void ArtNetServer::threadFunc(void *arg)
{
    ArtNetServer *self = (ArtNetServer*) arg;
    int fd, i;
    struct sockaddr_in sin;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    i = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof i);
    i = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &i, sizeof i);

    memset(&sin, 0, sizeof sin);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(6454);
    if (bind(fd, (const struct sockaddr*) &sin, sizeof sin)) {
        perror("Art-Net: Can't bind to UDP port");
        return;
    }

    if (self->mVerbose) {
        fprintf(stderr, "Art-Net: Listening for DMX input\n");
    }

    while (true) {
        Packet buffer;        
        int l = recv(fd, &buffer, sizeof buffer, 0);

        if (l < 12) {
            if (self->mVerbose) { 
                fprintf(stderr, "Art-Net: Received packet too small (%d bytes)\n", l);
            }
            continue;
        }

        if (memcmp(buffer.signature, "Art-Net", 8)) {
            if (self->mVerbose) { 
                fprintf(stderr, "Art-Net: Packet signature incorrect (%02x %02x %02x %02x ...)\n",
                    buffer.signature[0], buffer.signature[1], buffer.signature[2], buffer.signature[3]);
            }
            continue;
        }

        self->handlePacket(buffer, l);
    }
}

void ArtNetServer::handlePacket(Packet &p, int bufferLen)
{
    // Translate Art-Net header to OPC header

    if (p.opLow == 0x00 && p.opHigh == 0x50 && p.net == 0) {
        // ArtDMX packet with supported 8-bit universe number

        unsigned protoLen = ntohs(p.length);
        if (protoLen < 2 || protoLen > 512 || protoLen + 18 > bufferLen) {
            if (mVerbose) { 
                fprintf(stderr, "Art-Net: Bad packet lengths (UDP packet: %d bytes,  ArtDMX header: %d bytes)",
                    bufferLen, protoLen);
            }
            return;
        }

        // Copy out Art-Net header pieces
        uint8_t subUni = p.subUni;

        // New header in 
        p.msg.channel = subUni;
        p.msg.command = OPC::SetPixelColors;
        p.msg.setLength(protoLen);

        mOpcCallback(p.msg, mUserContext);
    }
}