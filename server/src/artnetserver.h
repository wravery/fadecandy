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

#pragma once
#include <stdint.h>
#include "libwebsockets.h"
#include "tinythread.h"
#include "opc.h"


class ArtNetServer {
public:
    ArtNetServer(OPC::callback_t opcCallback, void *context, bool verbose = false);

    // Start the event loop on a separate thread
    bool start();

private:
    OPC::callback_t mOpcCallback;
    void *mUserContext;
    tthread::thread *mThread;
    bool mVerbose;

    struct Packet {
        // Overlay the OPC and Art-Net packets in memory so that data[] matches
        union {
            // Art-Net header
            struct {
                uint8_t  signature[8];
                uint8_t  opLow;
                uint8_t  opHigh;
                uint16_t version;
                uint8_t  sequence;
                uint8_t  physical;
                uint8_t  subUni;
                uint8_t  net;
                uint16_t length;
            };

            // OPC packet
            struct {
                uint8_t _pad[14];
                OPC::Message msg;
            };
        };
    };

    static void threadFunc(void *arg);
    void handlePacket(Packet &p, int bufferLen);
};
