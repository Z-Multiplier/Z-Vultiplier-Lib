//MIT License

//Copyright (c) 2026 Z-Multiplier
#ifndef NETWORK_HPP
#define NETWORK_HPP
#include "Json.hpp"
#include "enet/enet.h"
#include <vector>
namespace Network{
    class Connection{
        ENetPeer* peer;
        public:
            Connection(ENetPeer* p):peer(p){};
            uint64_t getId()const;
            std::string getIp()const;
            void send(const std::vector<uint8_t>& data,int channel=0,bool reliable=true);
            void disconnect();
    };
    class Transport{
        std::vector<ENetEvent> pendingEvents;
        public:
            ENetHost* host=nullptr;
            bool initialize(bool isServer,int port,int maxClients);
            void update(float dt);

            bool send(ENetPeer* peer,const uint8_t* data,size_t len,bool reliable);

            const std::vector<ENetEvent>& GetPendingEvents()const{return pendingEvents;}
            void ClearEvents(){pendingEvents.clear();}
    };
    class Server{
        std::unique_ptr<Transport> trans;
        std::map<uint64_t,std::shared_ptr<Connection>> clients;
        public:
            virtual void OnConnect(ENetPeer* peer);
            virtual void OnReceive(ENetPeer* peer,const uint8_t* data,size_t len);
            virtual void OnDisconnect(ENetPeer* peer);
            void Start(int port,int maxClients);
            void Update(float dt);
            void Broadcast(const std::vector<uint8_t>& data,int channel=0);
            const std::map<uint64_t,std::shared_ptr<Connection>>& GetClients()const{return clients;}
    };
    class Client{
        std::unique_ptr<Transport> trans;
        public:
            std::shared_ptr<Connection> serverConn;
            virtual void onServerMessage(const uint8_t* data,size_t len);
            void connect(const std::string& ip,int port);
            void update(float dt);
            void sendToServer(const std::vector<uint8_t>& data,int channel=0,bool reliable=true);
            void disconnect();
    };
    class BitStream{
        public:
            BitStream()=default;
            explicit BitStream(const std::vector<uint8_t>& datas):data(datas){}

            void WriteBits(uint64_t value,uint8_t bitCount);
            void WriteBytes(const uint8_t* data,size_t len);
            
            void WriteInt(int32_t value){WriteBits(static_cast<uint64_t>(value),32);}
            void WriteUInt(uint32_t value){WriteBits(value,32);}
            void WriteFloat(float value);
            void WriteString(const std::string& str);
            void WriteBool(bool value){WriteBits(value?1:0,1);}

            uint64_t ReadBits(uint8_t bitCount);
            void ReadBytes(uint8_t* out,size_t len);
            
            int32_t ReadInt(){return static_cast<int32_t>(ReadBits(32));}
            uint32_t ReadUInt(){return static_cast<uint32_t>(ReadBits(32));}
            float ReadFloat();
            std::string ReadString();
            bool ReadBool(){return ReadBits(1)!=0;}

            const std::vector<uint8_t>& GetData()const{return data;}
            size_t GetSize()const{return data.size();}
            void Clear(){data.clear();readBitPos=0;}
            void ResetRead(){readBitPos=0;}

        private:
            std::vector<uint8_t> data;
            size_t bitPos=0;
            size_t readBitPos=0;
    };
}

#endif