//MIT License

//Copyright (c) 2026 Z-Multiplier
#include "Network.hpp"
#include "Logger.hpp"

namespace Network{
    uint64_t Connection::getId()const{
        return static_cast<uint64_t>(peer->connectID);
    }

    std::string Connection::getIp()const{
        char ip[64];
        enet_address_get_host_ip(&peer->address,ip,sizeof(ip));
        return std::string(ip);
    }

    void Connection::send(const std::vector<uint8_t>& data,int channel,bool reliable){
        if(!peer) return;
        
        uint32_t flags=reliable?ENET_PACKET_FLAG_RELIABLE:0;
        ENetPacket* packet=enet_packet_create(data.data(),data.size(),flags);
        if(packet){
            enet_peer_send(peer,channel,packet);
        }
    }

    void Connection::disconnect(){
        if(peer){
            enet_peer_disconnect(peer,0);
        }
    }

    bool Transport::initialize(bool isServer,int port,int maxClients){
        if(enet_initialize()!=0){
            Core::globalLogger.traceLog(
                Core::logger::LOG_ERROR,
                "[Network] ENet initialization failed",
                std::source_location::current()
            );
            return false;
        }

        ENetAddress address;
        address.host=ENET_HOST_ANY;
        address.port=static_cast<enet_uint16>(port);

        if(!isServer){
            host=enet_host_create(nullptr,1,2,0,0);
        }
        else{
            host=enet_host_create(&address,maxClients,2,0,0);
        }

        if(!host){
            Core::globalLogger.traceLog(
                Core::logger::LOG_ERROR,
                "[Network] Failed to create ENet host",
                std::source_location::current()
            );
            return false;
        }

        return true;
    }

    void Transport::update(float dt){
        if(!host) return;
        
        ENetEvent event;
        while(enet_host_service(host,&event,0)>0){
            pendingEvents.push_back(event);
        }
    }

    bool Transport::send(ENetPeer* peer,const uint8_t* data,size_t len,bool reliable){
        if(!host||!peer) return false;
        
        uint32_t flags=reliable?ENET_PACKET_FLAG_RELIABLE:0;
        ENetPacket* packet=enet_packet_create(data,len,flags);
        if(!packet) return false;
        
        return enet_peer_send(peer,0,packet)==0;
    }

    void Server::Start(int port,int maxClients){
        trans=std::make_unique<Transport>();
        if(!trans->initialize(true,port,maxClients)){
            Core::globalLogger.traceLog(
                Core::logger::LOG_ERROR,
                "[Server] Failed to start on port "+std::to_string(port),
                std::source_location::current()
            );
            return;
        }
        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "[Server] Started on port "+std::to_string(port),
            std::source_location::current()
        );
    }

    void Server::Update(float dt){
        if(!trans) return;

        trans->update(dt);

        const auto& events=trans->GetPendingEvents();
        for(const auto& ev:events){
            switch(ev.type){
                case ENET_EVENT_TYPE_CONNECT:
                    OnConnect(ev.peer);
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    OnReceive(ev.peer,ev.packet->data,ev.packet->dataLength);
                    enet_packet_destroy(ev.packet);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    OnDisconnect(ev.peer);
                    break;
                default:
                    break;
            }
        }

        trans->ClearEvents();
    }

    void Server::Broadcast(const std::vector<uint8_t>& data,int channel){
        if(trans&&trans->host){
            ENetPacket* packet=enet_packet_create(data.data(),data.size(),ENET_PACKET_FLAG_RELIABLE);
            if(packet){
                enet_host_broadcast(trans->host,channel,packet);
            }
        }
    }

    void Server::OnConnect(ENetPeer* peer){
        auto conn=std::make_shared<Connection>(peer);
        uint64_t id=conn->getId();
        clients[id]=conn;
        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "[Server] Client "+std::to_string(id)+" connected from "+conn->getIp(),
            std::source_location::current()
        );
    }

    void Server::OnReceive(ENetPeer* peer,const uint8_t* data,size_t len){
        uint64_t id=peer->connectID;
    }

    void Server::OnDisconnect(ENetPeer* peer){
        uint64_t id=peer->connectID;
        auto it=clients.find(id);
        if(it!=clients.end()){
            Core::globalLogger.traceLog(
                Core::logger::LOG_INFO,
                "[Server] Client "+std::to_string(id)+" disconnected",
                std::source_location::current()
            );
            clients.erase(it);
        }
    }

    void Client::connect(const std::string& ip,int port){
        trans=std::make_unique<Transport>();
        if(!trans->initialize(false,0,1)){
            Core::globalLogger.traceLog(
                Core::logger::LOG_ERROR,
                "[Client] Failed to initialize",
                std::source_location::current()
            );
            return;
        }

        ENetAddress address;
        enet_address_set_host(&address,ip.c_str());
        address.port=static_cast<enet_uint16>(port);

        ENetPeer* peer=enet_host_connect(trans->host,&address,2,0);
        if(!peer){
            Core::globalLogger.traceLog(
                Core::logger::LOG_ERROR,
                "[Client] Failed to connect to "+ip+" : "+std::to_string(port),
                std::source_location::current()
            );
            return;
        }

        serverConn=std::make_shared<Connection>(peer);
        Core::globalLogger.traceLog(
            Core::logger::LOG_INFO,
            "[Client] Connecting to "+ip+":"+std::to_string(port)+"...",
            std::source_location::current()
        );
    }

    void Client::update(float dt){
        if(!trans) return;

        trans->update(dt);

        const auto& events=trans->GetPendingEvents();
        for(const auto& ev:events){
            switch(ev.type){
                case ENET_EVENT_TYPE_CONNECT:
                    Core::globalLogger.traceLog(
                        Core::logger::LOG_INFO,
                        "[Client] Connected to server!",
                        std::source_location::current()
                    );
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    onServerMessage(ev.packet->data,ev.packet->dataLength);
                    enet_packet_destroy(ev.packet);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    Core::globalLogger.traceLog(
                        Core::logger::LOG_INFO,
                        "[Client] Disconnected from server",
                        std::source_location::current()
                    );
                    serverConn.reset();
                    break;
                default:
                    break;
            }
        }

        trans->ClearEvents();
    }

    void Client::sendToServer(const std::vector<uint8_t>& data,int channel,bool reliable){
        if(serverConn){
            serverConn->send(data,channel,reliable);
        }
    }

    void Client::disconnect(){
        if(serverConn){
            serverConn->disconnect();
            serverConn.reset();
        }
    }

    void Client::onServerMessage(const uint8_t* data,size_t len){
    }

    void BitStream::WriteBits(uint64_t value,uint8_t bitCount){
        if(bitCount==0) return;
        
        size_t currentByte=bitPos>>3;
        uint8_t bitOffset=bitPos&7;
        
        size_t neededBytes=(bitPos+bitCount+7)>>3;
        if(data.size()<neededBytes){
            data.resize(neededBytes,0);
        }
        
        for(int i=0;i<bitCount;++i){
            uint8_t bit=(value>>(bitCount-1-i))&1;
            if(bit){
                data[currentByte]|=(1<<(7-bitOffset));
            }
            else{
                data[currentByte]&=~(1<<(7-bitOffset));
            }
            bitOffset++;
            if(bitOffset==8){
                bitOffset=0;
                currentByte++;
            }
        }
        bitPos+=bitCount;
    }

    void BitStream::WriteBytes(const uint8_t* datas,size_t len){
        if(len==0) return;
        if(bitPos&7){
            uint8_t align=8-(bitPos&7);
            WriteBits(0,align);
        }
        size_t bytePos=bitPos>>3;
        data.resize(bytePos+len);
        memcpy(&data[bytePos],datas,len);
        bitPos+=len*8;
    }

    void BitStream::WriteFloat(float value){
        uint32_t bits;
        memcpy(&bits,&value,sizeof(bits));
        WriteBits(bits,32);
    }

    void BitStream::WriteString(const std::string& str){
        WriteUInt(static_cast<uint32_t>(str.size()));
        WriteBytes(reinterpret_cast<const uint8_t*>(str.data()),str.size());
    }

    uint64_t BitStream::ReadBits(uint8_t bitCount){
        if(bitCount==0) return 0;
        
        uint64_t result=0;
        size_t currentByte=readBitPos>>3;
        uint8_t bitOffset=readBitPos&7;
        
        for(int i=0;i<bitCount;++i){
            uint8_t bit=(data[currentByte]>>(7-bitOffset))&1;
            result=(result<<1)|bit;
            bitOffset++;
            if(bitOffset==8){
                bitOffset=0;
                currentByte++;
            }
        }
        readBitPos+=bitCount;
        return result;
    }

    void BitStream::ReadBytes(uint8_t* out,size_t len){
        if(len==0) return;
        if(readBitPos&7){
            uint8_t align=8-(readBitPos&7);
            ReadBits(align);
        }
        size_t bytePos=readBitPos>>3;
        memcpy(out,&data[bytePos],len);
        readBitPos+=len*8;
    }

    float BitStream::ReadFloat(){
        uint32_t bits=static_cast<uint32_t>(ReadBits(32));
        float result;
        memcpy(&result,&bits,sizeof(result));
        return result;
    }

    std::string BitStream::ReadString(){
        uint32_t len=ReadUInt();
        std::string result;
        result.resize(len);
        if(len>0){
            ReadBytes(reinterpret_cast<uint8_t*>(&result[0]),len);
        }
        return result;
    }
}