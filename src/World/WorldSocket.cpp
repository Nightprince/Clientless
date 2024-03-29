/*
 * Copyright (C) 2015 Dehravor <dehravor@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "WorldSocket.h"
#include "WorldSession.h"

#ifndef _WIN32
    #include <netinet/in.h>
#endif

WorldSocket::WorldSocket(WorldSession* session) : session_(session)
{
}

WorldSocket::~WorldSocket()
{
    if (IsConnected())
        Disconnect();

    if (senderThread_.joinable())
        senderThread_.join();

    if (receiverThread_.joinable())
        receiverThread_.join();
}

bool WorldSocket::Connect(std::string address)
{
    assert(!IsConnected());

    if (senderThread_.joinable())
        senderThread_.join();

    if (receiverThread_.joinable())
        receiverThread_.join();

    if (!TCPSocket::Connect(address))
        return false;

    packetCrypt_.Reset();

    senderThread_ = std::thread(&WorldSocket::RunSenderThread, this);
    receiverThread_ = std::thread(&WorldSocket::RunReceiverThread, this);
    return true;
}

void WorldSocket::Disconnect()
{
    TCPSocket::Disconnect();
   
    std::lock_guard<std::recursive_mutex> sendLock(sendMutex_);

    while (!sendQueue_.empty())
        sendQueue_.pop();

    std::lock_guard<std::recursive_mutex> receiveLock(receiveMutex_);

    while (!receiveQueue_.empty())
        receiveQueue_.pop();
}

void WorldSocket::EnqueuePacket(WorldPacket &packet)
{
    std::shared_ptr<WorldPacket> copy(new WorldPacket(packet));

    std::lock_guard<std::recursive_mutex> lock(sendMutex_);
    sendQueue_.push(copy);
}

std::shared_ptr<WorldPacket> WorldSocket::GetNextPacket()
{
    if (receiveQueue_.empty())
        return nullptr;

    std::lock_guard<std::recursive_mutex> lock(receiveMutex_);
    std::shared_ptr<WorldPacket> packet = receiveQueue_.front();
    receiveQueue_.pop();

    return packet;
}

void WorldSocket::RunSenderThread()
{
    while (IsConnected())
    {
        if (!sendQueue_.empty())
        {
            // Acquire next packet
            std::lock_guard<std::recursive_mutex> lock(sendMutex_);
            std::shared_ptr<WorldPacket> packet = sendQueue_.front();
            sendQueue_.pop();

            ByteBuffer prepared;

            uint16 size = htons(packet->size() + 4);
            uint32 opcode = uint32(packet->GetOpcode());

            packetCrypt_.EncryptSend((uint8*)&size, 2);
            packetCrypt_.EncryptSend((uint8*)&opcode, 4);

            prepared << size;
            prepared << opcode;

            if (!packet->empty())
                prepared.append(packet->contents(), packet->size());

            Send(prepared.contents(), prepared.size());

            if (packet->GetOpcode() == CMSG_AUTH_SESSION)
                packetCrypt_.Initialize(&session_->session_->GetKey());
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void WorldSocket::RunReceiverThread()
{
    uint8 header[5];
    uint32 size;
    Opcodes opcode;

    while (IsConnected())
    {
        // Read normal header (4 bytes)
        Read((char*)&header[0], 4);

        if (!IsConnected())
            break;

        packetCrypt_.DecryptReceived(&header[0], 4);

        // Read additional header byte if necessary (1 byte)
        if (header[0] & 0x80)
        {
            Read((char*)&header[4], 1);

            if (!IsConnected())
                break;

            packetCrypt_.DecryptReceived(&header[4], 1);
        }

        // Calculate size and opcode
        if (header[0] & 0x80)
        {
            size = ((header[0] & 0x7F) << 16) | (header[1] << 8) | header[2];
            opcode = static_cast<Opcodes>(header[3] | (header[4] << 8));
        }
        else
        {
            size = (header[0] << 8) | header[1];
            opcode = static_cast<Opcodes>(header[2] | (header[3] << 8));
        }

        size -= sizeof(Opcodes);

        // Read body
        std::shared_ptr<WorldPacket> packet(new WorldPacket(opcode, size));
        packet->resize(size);

        if (size)
            Read(reinterpret_cast<char*>(packet->contents()), size);

        if (!IsConnected())
            break;

        std::lock_guard<std::recursive_mutex> lock(receiveMutex_);
        receiveQueue_.push(packet);
    }

    print("%s", "Disconnected from the server.");
}